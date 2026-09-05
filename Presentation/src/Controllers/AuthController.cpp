#include "Presentation/Controllers/AuthController.h"
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

std::string getDeviceName(const crow::request& req) {
    std::string dev = req.get_header_value("X-Device-Name");
    if (!dev.empty()) return dev;
    std::string ua = getUserAgent(req);
    if (ua.find("Windows") != std::string::npos) return "Windows PC";
    if (ua.find("Macintosh") != std::string::npos) return "Mac";
    if (ua.find("iPhone") != std::string::npos) return "iPhone";
    if (ua.find("Android") != std::string::npos) return "Android Device";
    if (ua.find("Linux") != std::string::npos) return "Linux Workstation";
    return "Web Client";
}

} // anonymous namespace

AuthController::AuthController(
    std::shared_ptr<Application::UseCases::AuthUseCases> authUseCases,
    std::shared_ptr<Application::Security::IKeyManager> keyManager,
    std::shared_ptr<Application::Security::IJwtService> jwtService,
    std::shared_ptr<Domain::Repositories::ISessionRepository> sessionRepo,
    std::shared_ptr<Domain::Repositories::IUserRepository> userRepo
)   : m_authUseCases(std::move(authUseCases)),
      m_keyManager(std::move(keyManager)),
      m_jwtService(std::move(jwtService)),
      m_sessionRepo(std::move(sessionRepo)),
      m_userRepo(std::move(userRepo)) {}

crow::response AuthController::registerUser(const crow::request& req) const {
    auto parsed = Common::HttpResponseHelper::parseRegisterRequest(req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    auto result = m_authUseCases->registerUser(parsed.value());
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response AuthController::login(const crow::request& req) const {
    auto parsed = Common::HttpResponseHelper::parseLoginRequest(req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    std::string ip = getClientIp(req);
    std::string ua = getUserAgent(req);
    std::string dev = getDeviceName(req);

    auto result = m_authUseCases->login(parsed.value(), ip, ua, dev);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response AuthController::googleLogin(const crow::request& req) const {
    auto parsed = Common::HttpResponseHelper::parseGoogleLoginRequest(req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    std::string ip = getClientIp(req);
    std::string ua = getUserAgent(req);
    std::string dev = parsed.value().deviceName.empty() ? getDeviceName(req) : parsed.value().deviceName;
    auto loginReq = parsed.value();
    loginReq.deviceName = dev;

    auto result = m_authUseCases->loginWithGoogle(loginReq, ip, ua);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response AuthController::getGoogleAuthUrl(const crow::request& req) const {
    std::string state;
    if (req.url_params.get("state") != nullptr) {
        state = req.url_params.get("state");
    }

    auto result = m_authUseCases->getGoogleAuthUrl(state);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response AuthController::googleCallback(const crow::request& req) const {
    const char* code = req.url_params.get("code");
    if (!code || std::strlen(code) == 0) {
        const char* error = req.url_params.get("error");
        std::string errMsg = error ? error : "Missing authorization code from Google.";
        return Common::HttpResponseHelper::error(400, "Google OAuth failed", {errMsg});
    }

    const char* state = req.url_params.get("state");
    std::string stateStr = (state && std::strlen(state) > 0) ? state : "";

    Application::DTOs::GoogleLoginRequest loginReq{
        .idToken = "",
        .code = code,
        .codeVerifier = "",
        .state = std::move(stateStr),
        .deviceName = getDeviceName(req),
        .clientType = "web-callback"
    };

    std::string ip = getClientIp(req);
    std::string ua = getUserAgent(req);

    auto result = m_authUseCases->loginWithGoogle(loginReq, ip, ua);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response AuthController::setPassword(const crow::request& req) const {
    auto authUserRes = Common::HttpResponseHelper::extractAuthenticatedUser(
        req, *m_jwtService, *m_sessionRepo, *m_userRepo
    );
    if (authUserRes.isErr()) {
        return Common::HttpResponseHelper::error(authUserRes.error());
    }

    auto parsed = Common::HttpResponseHelper::parseSetPasswordRequest(req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    const auto& user = authUserRes.value();
    auto result = m_authUseCases->setPassword(user.id, parsed.value());
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response AuthController::refresh(const crow::request& req) const {
    auto parsed = Common::HttpResponseHelper::parseRefreshTokenRequest(req);
    if (parsed.isErr()) {
        return Common::HttpResponseHelper::error(parsed.error());
    }

    std::string ip = getClientIp(req);
    std::string ua = getUserAgent(req);

    auto result = m_authUseCases->refresh(parsed.value(), ip, ua);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response AuthController::logout(const crow::request& req) const {
    auto authUserRes = Common::HttpResponseHelper::extractAuthenticatedUser(
        req, *m_jwtService, *m_sessionRepo, *m_userRepo
    );
    if (authUserRes.isErr()) {
        return Common::HttpResponseHelper::error(authUserRes.error());
    }

    std::string ip = getClientIp(req);
    std::string ua = getUserAgent(req);
    const auto& user = authUserRes.value();

    auto result = m_authUseCases->logout(user.sessionId, user.id, ip, ua);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response AuthController::logoutAll(const crow::request& req) const {
    auto authUserRes = Common::HttpResponseHelper::extractAuthenticatedUser(
        req, *m_jwtService, *m_sessionRepo, *m_userRepo
    );
    if (authUserRes.isErr()) {
        return Common::HttpResponseHelper::error(authUserRes.error());
    }

    std::string ip = getClientIp(req);
    std::string ua = getUserAgent(req);
    const auto& user = authUserRes.value();

    auto result = m_authUseCases->logoutAll(user.id, ip, ua);
    if (result.isErr()) {
        return Common::HttpResponseHelper::error(result.error());
    }

    return Common::HttpResponseHelper::success(result.value());
}

crow::response AuthController::getJwks() const {
    std::string jwks = m_keyManager->getJwksJson();
    crow::response res(200, jwks);
    res.set_header("Content-Type", "application/json");
    res.set_header("Cache-Control", "public, max-age=3600");
    return res;
}

} // namespace Presentation::Controllers
