#include "Presentation/Controllers/SessionController.h"
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

SessionController::SessionController(
    std::shared_ptr<Application::UseCases::SessionUseCases> sessionUseCases,
    std::shared_ptr<Application::Security::IJwtService> jwtService,
    std::shared_ptr<Domain::Repositories::ISessionRepository> sessionRepo,
    std::shared_ptr<Domain::Repositories::IUserRepository> userRepo
)   : m_sessionUseCases(std::move(sessionUseCases)),
      m_jwtService(std::move(jwtService)),
      m_sessionRepo(std::move(sessionRepo)),
      m_userRepo(std::move(userRepo)) {}

crow::response SessionController::getSessions(const crow::request& req) const {
    auto authUserRes = Common::HttpResponseHelper::extractAuthenticatedUser(
        req, *m_jwtService, *m_sessionRepo, *m_userRepo
    );
    if (authUserRes.isErr()) {
        return Common::HttpResponseHelper::error(authUserRes.error());
    }

    const auto& user = authUserRes.value();
    auto result = m_sessionUseCases->getUserSessions(user.id, user.sessionId);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response SessionController::revokeSession(const crow::request& req, const std::string& sessionId) const {
    auto authUserRes = Common::HttpResponseHelper::extractAuthenticatedUser(
        req, *m_jwtService, *m_sessionRepo, *m_userRepo
    );
    if (authUserRes.isErr()) {
        return Common::HttpResponseHelper::error(authUserRes.error());
    }

    const auto& user = authUserRes.value();
    std::string ip = getClientIp(req);
    std::string ua = getUserAgent(req);

    auto result = m_sessionUseCases->revokeUserSession(user.id, sessionId, ip, ua);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

} // namespace Presentation::Controllers
