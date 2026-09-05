#pragma once

#include "Application/Security/IKeyManager.h"
#include "Application/Security/IJwtService.h"
#include "Application/UseCases/AuthUseCases.h"
#include "Domain/Repositories/ISessionRepository.h"
#include "Domain/Repositories/IUserRepository.h"
#include <crow.h>
#include <memory>

namespace Presentation::Controllers {

class AuthController {
public:
    AuthController(
        std::shared_ptr<Application::UseCases::AuthUseCases> authUseCases,
        std::shared_ptr<Application::Security::IKeyManager> keyManager,
        std::shared_ptr<Application::Security::IJwtService> jwtService,
        std::shared_ptr<Domain::Repositories::ISessionRepository> sessionRepo,
        std::shared_ptr<Domain::Repositories::IUserRepository> userRepo
    );

    crow::response registerUser(const crow::request& req) const;
    crow::response login(const crow::request& req) const;
    crow::response googleLogin(const crow::request& req) const;
    crow::response getGoogleAuthUrl(const crow::request& req) const;
    crow::response googleCallback(const crow::request& req) const;
    crow::response setPassword(const crow::request& req) const;
    crow::response refresh(const crow::request& req) const;
    crow::response logout(const crow::request& req) const;
    crow::response logoutAll(const crow::request& req) const;
    crow::response getJwks() const;

private:
    std::shared_ptr<Application::UseCases::AuthUseCases> m_authUseCases;
    std::shared_ptr<Application::Security::IKeyManager> m_keyManager;
    std::shared_ptr<Application::Security::IJwtService> m_jwtService;
    std::shared_ptr<Domain::Repositories::ISessionRepository> m_sessionRepo;
    std::shared_ptr<Domain::Repositories::IUserRepository> m_userRepo;
};

} // namespace Presentation::Controllers
