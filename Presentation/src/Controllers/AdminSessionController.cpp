#include "Presentation/Controllers/AdminSessionController.h"
#include "Presentation/Common/HttpResponseHelper.h"

namespace Presentation::Controllers {

namespace {

std::string getClientIp(const crow::request& req) {
    std::string forwarded = req.get_header_value("X-Forwarded-For");
    if (!forwarded.empty()) {
        size_t comma = forwarded.find(',');
        return comma == std::string::npos ? forwarded : forwarded.substr(0, comma);
    }
    return req.remote_ip_address.empty() ? "127.0.0.1" : req.remote_ip_address;
}

std::string getUserAgent(const crow::request& req) {
    std::string ua = req.get_header_value("User-Agent");
    return ua.empty() ? "Unknown Client" : ua;
}

} // anonymous namespace

AdminSessionController::AdminSessionController(
    std::shared_ptr<Application::UseCases::SessionUseCases> sessionUseCases,
    std::shared_ptr<Application::Security::IJwtService> jwtService,
    std::shared_ptr<Domain::Repositories::ISessionRepository> sessionRepo,
    std::shared_ptr<Domain::Repositories::IUserRepository> userRepo
)   : m_sessionUseCases(std::move(sessionUseCases)),
      m_jwtService(std::move(jwtService)),
      m_sessionRepo(std::move(sessionRepo)),
      m_userRepo(std::move(userRepo)) {}

crow::response AdminSessionController::getSessions(const crow::request& req) const {
    auto authUserRes = Common::HttpResponseHelper::extractAuthenticatedUser(
        req, *m_jwtService, *m_sessionRepo, *m_userRepo
    );
    if (authUserRes.isErr()) {
        return Common::HttpResponseHelper::error(authUserRes.error());
    }

    auto userOpt = m_userRepo->findById(authUserRes.value().id);
    if (!userOpt.has_value()) {
        return Common::HttpResponseHelper::error(401, "User record not found");
    }

    int limit = 50;
    int offset = 0;
    if (req.url_params.get("limit") != nullptr) {
        try { limit = std::stoi(req.url_params.get("limit")); } catch (...) {}
    }
    if (req.url_params.get("offset") != nullptr) {
        try { offset = std::stoi(req.url_params.get("offset")); } catch (...) {}
    }

    std::optional<std::string_view> filterUserId = std::nullopt;
    if (req.url_params.get("user_id") != nullptr) {
        filterUserId = req.url_params.get("user_id");
    }

    std::optional<std::string_view> filterStatus = std::nullopt;
    if (req.url_params.get("status") != nullptr) {
        filterStatus = req.url_params.get("status");
    }

    std::optional<std::string_view> filterIp = std::nullopt;
    if (req.url_params.get("ip") != nullptr) {
        filterIp = req.url_params.get("ip");
    }

    auto result = m_sessionUseCases->getAdminSessions(
        *userOpt, limit, offset, filterUserId, filterStatus, filterIp
    );
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response AdminSessionController::revokeSession(const crow::request& req, const std::string& sessionId) const {
    auto authUserRes = Common::HttpResponseHelper::extractAuthenticatedUser(
        req, *m_jwtService, *m_sessionRepo, *m_userRepo
    );
    if (authUserRes.isErr()) {
        return Common::HttpResponseHelper::error(authUserRes.error());
    }

    auto userOpt = m_userRepo->findById(authUserRes.value().id);
    if (!userOpt.has_value()) {
        return Common::HttpResponseHelper::error(401, "User record not found");
    }

    std::string ip = getClientIp(req);
    std::string ua = getUserAgent(req);
    std::string reason = "Administrative revocation";

    if (req.url_params.get("reason") != nullptr) {
        reason = req.url_params.get("reason");
    } else if (!req.body.empty()) {
        auto json = crow::json::load(req.body);
        if (json && json.has("reason")) {
            reason = std::string(json["reason"].s());
        }
    }

    auto result = m_sessionUseCases->revokeAdminSession(*userOpt, sessionId, reason, ip, ua);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response AdminSessionController::revokeAllUserSessions(const crow::request& req, const std::string& userId) const {
    auto authUserRes = Common::HttpResponseHelper::extractAuthenticatedUser(
        req, *m_jwtService, *m_sessionRepo, *m_userRepo
    );
    if (authUserRes.isErr()) {
        return Common::HttpResponseHelper::error(authUserRes.error());
    }

    auto userOpt = m_userRepo->findById(authUserRes.value().id);
    if (!userOpt.has_value()) {
        return Common::HttpResponseHelper::error(401, "User record not found");
    }

    std::string ip = getClientIp(req);
    std::string ua = getUserAgent(req);
    std::string reason = "Administrative revocation of all sessions";

    auto parsed = Common::HttpResponseHelper::parseAdminRevokeUserSessionsRequest(req);
    if (parsed.isOk()) {
        reason = parsed.value().reason;
    }

    auto result = m_sessionUseCases->revokeAllUserSessionsAdmin(*userOpt, userId, reason, ip, ua);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

} // namespace Presentation::Controllers
