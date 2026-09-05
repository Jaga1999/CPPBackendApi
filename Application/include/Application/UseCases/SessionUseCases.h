#pragma once

#include "Application/Common/ApiResponse.h"
#include "Application/DTOs/AuthDtos.h"
#include "Application/Security/AuthorizationService.h"
#include "Domain/Common/Result.h"
#include "Domain/Entities/User.h"
#include "Domain/Repositories/IAuditLogRepository.h"
#include "Domain/Repositories/ISessionRepository.h"
#include "Domain/Repositories/IUserRepository.h"
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Application::UseCases {

class SessionUseCases {
public:
    SessionUseCases(
        std::shared_ptr<Domain::Repositories::ISessionRepository> sessionRepo,
        std::shared_ptr<Domain::Repositories::IUserRepository> userRepo,
        std::shared_ptr<Domain::Repositories::IAuditLogRepository> auditRepo,
        std::shared_ptr<Security::AuthorizationService> authService
    );

    Domain::Common::Result<Common::ApiResponse<std::vector<DTOs::SessionResponse>>> getUserSessions(
        std::string_view userId,
        std::string_view currentSessionId
    );

    Domain::Common::Result<Common::ApiResponse<void>> revokeUserSession(
        std::string_view userId,
        std::string_view sessionId,
        std::string_view ipAddress,
        std::string_view userAgent
    );

    Domain::Common::Result<Common::ApiResponse<std::vector<DTOs::SessionResponse>>> getAdminSessions(
        const Domain::Entities::User& adminUser,
        int limit,
        int offset,
        std::optional<std::string_view> filterUserId = std::nullopt,
        std::optional<std::string_view> filterStatus = std::nullopt,
        std::optional<std::string_view> filterIp = std::nullopt
    );

    Domain::Common::Result<Common::ApiResponse<void>> revokeAdminSession(
        const Domain::Entities::User& adminUser,
        std::string_view sessionId,
        std::string_view reason,
        std::string_view ipAddress,
        std::string_view userAgent
    );

    Domain::Common::Result<Common::ApiResponse<void>> revokeAllUserSessionsAdmin(
        const Domain::Entities::User& adminUser,
        std::string_view targetUserId,
        std::string_view reason,
        std::string_view ipAddress,
        std::string_view userAgent
    );

private:
    std::shared_ptr<Domain::Repositories::ISessionRepository> m_sessionRepo;
    std::shared_ptr<Domain::Repositories::IUserRepository> m_userRepo;
    std::shared_ptr<Domain::Repositories::IAuditLogRepository> m_auditRepo;
    std::shared_ptr<Security::AuthorizationService> m_authService;
};

} // namespace Application::UseCases
