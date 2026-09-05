#pragma once

#include "Application/Security/IJwtService.h"
#include "Application/UseCases/SessionUseCases.h"
#include "Domain/Repositories/ISessionRepository.h"
#include "Domain/Repositories/IUserRepository.h"
#include <crow.h>
#include <memory>
#include <string>

namespace Presentation::Controllers {

class SessionController {
public:
    SessionController(
        std::shared_ptr<Application::UseCases::SessionUseCases> sessionUseCases,
        std::shared_ptr<Application::Security::IJwtService> jwtService,
        std::shared_ptr<Domain::Repositories::ISessionRepository> sessionRepo,
        std::shared_ptr<Domain::Repositories::IUserRepository> userRepo
    );

    crow::response getSessions(const crow::request& req) const;
    crow::response revokeSession(const crow::request& req, const std::string& sessionId) const;

private:
    std::shared_ptr<Application::UseCases::SessionUseCases> m_sessionUseCases;
    std::shared_ptr<Application::Security::IJwtService> m_jwtService;
    std::shared_ptr<Domain::Repositories::ISessionRepository> m_sessionRepo;
    std::shared_ptr<Domain::Repositories::IUserRepository> m_userRepo;
};

} // namespace Presentation::Controllers
