#include "Application/UseCases/SessionUseCases.h"
#include <chrono>

namespace Application::UseCases {

namespace {

std::string formatIsoTimestamp(std::chrono::system_clock::time_point tp) {
    const auto timeT = std::chrono::system_clock::to_time_t(tp);
    std::tm tmBuffer{};
#if defined(_WIN32)
    gmtime_s(&tmBuffer, &timeT);
#else
    gmtime_r(&timeT, &tmBuffer);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmBuffer);
    return std::string(buf);
}

DTOs::SessionResponse mapToSessionResponse(const Domain::Entities::Session& s, std::string_view currentSid) {
    std::string status = "active";
    if (s.isRevoked()) {
        status = "revoked";
    } else if (s.isExpired()) {
        status = "expired";
    }

    return DTOs::SessionResponse{
        .id = s.id,
        .userId = s.userId,
        .device = s.deviceName.empty() ? s.clientType : s.deviceName,
        .ipAddress = s.ipAddress,
        .jti = s.jti,
        .createdAt = formatIsoTimestamp(s.createdAt),
        .lastSeenAt = formatIsoTimestamp(s.lastSeenAt),
        .expiresAt = formatIsoTimestamp(s.expiresAt),
        .current = (!currentSid.empty() && s.id == currentSid),
        .status = std::move(status)
    };
}

} // anonymous namespace

SessionUseCases::SessionUseCases(
    std::shared_ptr<Domain::Repositories::ISessionRepository> sessionRepo,
    std::shared_ptr<Domain::Repositories::IUserRepository> userRepo,
    std::shared_ptr<Domain::Repositories::IAuditLogRepository> auditRepo,
    std::shared_ptr<Security::AuthorizationService> authService
)   : m_sessionRepo(std::move(sessionRepo)),
      m_userRepo(std::move(userRepo)),
      m_auditRepo(std::move(auditRepo)),
      m_authService(std::move(authService)) {}

Domain::Common::Result<Common::ApiResponse<std::vector<DTOs::SessionResponse>>> SessionUseCases::getUserSessions(
    std::string_view userId,
    std::string_view currentSessionId
) {
    auto sessions = m_sessionRepo->findUserSessions(userId, true);
    std::vector<DTOs::SessionResponse> list;
    list.reserve(sessions.size());

    for (const auto& s : sessions) {
        list.push_back(mapToSessionResponse(s, currentSessionId));
    }

    return Domain::Common::Result<Common::ApiResponse<std::vector<DTOs::SessionResponse>>>::ok(
        Common::ApiResponse<std::vector<DTOs::SessionResponse>>::ok(std::move(list), "Sessions retrieved successfully")
    );
}

Domain::Common::Result<Common::ApiResponse<void>> SessionUseCases::revokeUserSession(
    std::string_view userId,
    std::string_view sessionId,
    std::string_view ipAddress,
    std::string_view userAgent
) {
    auto sessionOpt = m_sessionRepo->findById(sessionId);
    if (!sessionOpt.has_value()) {
        sessionOpt = m_sessionRepo->findByJti(sessionId);
    }
    if (!sessionOpt.has_value()) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{.message = "Session not found", .statusCode = 404, .details = {"Session or JWT not found."}}
        );
    }

    // IDOR Defense: user may only revoke their own session
    if (sessionOpt->userId != userId) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{.message = "Access denied", .statusCode = 403, .details = {"You do not have permission to revoke this session."}}
        );
    }

    if (sessionOpt->isRevoked()) {
        return Domain::Common::Result<Common::ApiResponse<void>>::ok(
            Common::ApiResponse<void>::ok("Session was already revoked")
        );
    }

    m_sessionRepo->revokeSession(sessionOpt->id, "User terminated session");

    m_auditRepo->record(Domain::Entities::AuditLog{
        .eventType = "SESSION_REVOKED",
        .userId = std::string(userId),
        .sessionId = sessionOpt->id,
        .ipAddress = std::string(ipAddress),
        .userAgent = std::string(userAgent),
        .reason = "Self session termination"
    });

    return Domain::Common::Result<Common::ApiResponse<void>>::ok(
        Common::ApiResponse<void>::ok("Session revoked successfully")
    );
}

Domain::Common::Result<Common::ApiResponse<std::vector<DTOs::SessionResponse>>> SessionUseCases::getAdminSessions(
    const Domain::Entities::User& adminUser,
    int limit,
    int offset,
    std::optional<std::string_view> filterUserId,
    std::optional<std::string_view> filterStatus,
    std::optional<std::string_view> filterIp
) {
    if (!m_authService->hasPermission(adminUser, Domain::Common::Permission::SessionReadAll)) {
        return Domain::Common::Result<Common::ApiResponse<std::vector<DTOs::SessionResponse>>>::err(
            Domain::Common::DomainError{.message = "Forbidden", .statusCode = 403, .details = {"Administrative privilege required."}}
        );
    }

    if (limit <= 0) limit = 50;
    if (limit > 200) limit = 200;
    if (offset < 0) offset = 0;

    auto sessions = m_sessionRepo->findAllSessions(limit, offset, filterUserId, filterStatus, filterIp);
    std::vector<DTOs::SessionResponse> list;
    list.reserve(sessions.size());

    for (const auto& s : sessions) {
        list.push_back(mapToSessionResponse(s, ""));
    }

    return Domain::Common::Result<Common::ApiResponse<std::vector<DTOs::SessionResponse>>>::ok(
        Common::ApiResponse<std::vector<DTOs::SessionResponse>>::ok(std::move(list), "Admin sessions retrieved successfully")
    );
}

Domain::Common::Result<Common::ApiResponse<void>> SessionUseCases::revokeAdminSession(
    const Domain::Entities::User& adminUser,
    std::string_view sessionId,
    std::string_view reason,
    std::string_view ipAddress,
    std::string_view userAgent
) {
    if (!m_authService->hasPermission(adminUser, Domain::Common::Permission::SessionRevokeAll)) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{.message = "Forbidden", .statusCode = 403, .details = {"Administrative privilege required."}}
        );
    }

    auto sessionOpt = m_sessionRepo->findById(sessionId);
    if (!sessionOpt.has_value()) {
        sessionOpt = m_sessionRepo->findByJti(sessionId);
    }
    if (!sessionOpt.has_value()) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{.message = "Session not found", .statusCode = 404, .details = {"Target session or JWT does not exist."}}
        );
    }

    std::string revocationReason = reason.empty() ? "Revoked by administrator" : std::string(reason);
    m_sessionRepo->revokeSession(sessionOpt->id, revocationReason);

    m_auditRepo->record(Domain::Entities::AuditLog{
        .eventType = "SESSION_REVOKED_BY_ADMIN",
        .userId = sessionOpt->userId,
        .adminUserId = adminUser.id,
        .sessionId = sessionOpt->id,
        .ipAddress = std::string(ipAddress),
        .userAgent = std::string(userAgent),
        .reason = revocationReason
    });

    return Domain::Common::Result<Common::ApiResponse<void>>::ok(
        Common::ApiResponse<void>::ok("Session terminated by administrator")
    );
}

Domain::Common::Result<Common::ApiResponse<void>> SessionUseCases::revokeAllUserSessionsAdmin(
    const Domain::Entities::User& adminUser,
    std::string_view targetUserId,
    std::string_view reason,
    std::string_view ipAddress,
    std::string_view userAgent
) {
    if (!m_authService->hasPermission(adminUser, Domain::Common::Permission::SessionRevokeAll)) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{.message = "Forbidden", .statusCode = 403, .details = {"Administrative privilege required."}}
        );
    }

    std::string revocationReason = reason.empty() ? "All sessions revoked by administrator" : std::string(reason);
    size_t count = m_sessionRepo->revokeAllUserSessions(targetUserId, revocationReason);

    m_auditRepo->record(Domain::Entities::AuditLog{
        .eventType = "ALL_SESSIONS_REVOKED",
        .userId = std::string(targetUserId),
        .adminUserId = adminUser.id,
        .ipAddress = std::string(ipAddress),
        .userAgent = std::string(userAgent),
        .reason = revocationReason + ". Count: " + std::to_string(count)
    });

    return Domain::Common::Result<Common::ApiResponse<void>>::ok(
        Common::ApiResponse<void>::ok("All user sessions revoked by administrator. Count: " + std::to_string(count))
    );
}

} // namespace Application::UseCases
