#pragma once

#include "Application/Common/ApiResponse.h"
#include "Application/DTOs/AuthDtos.h"
#include "Application/DTOs/GoogleAuthDtos.h"
#include "Application/Security/IGoogleAuthService.h"
#include "Application/Security/IJwtService.h"
#include "Application/Security/IPasswordHasher.h"
#include "Application/Security/ITokenGenerator.h"
#include "Domain/Common/Result.h"
#include "Domain/Repositories/IAuditLogRepository.h"
#include "Domain/Repositories/ICacheRepository.h"
#include "Domain/Repositories/ISessionRepository.h"
#include "Domain/Repositories/IUserRepository.h"
#include <memory>
#include <string>
#include <string_view>

namespace Application::UseCases {

class AuthUseCases {
public:
    AuthUseCases(
        std::shared_ptr<Domain::Repositories::IUserRepository> userRepo,
        std::shared_ptr<Domain::Repositories::ISessionRepository> sessionRepo,
        std::shared_ptr<Domain::Repositories::IAuditLogRepository> auditRepo,
        std::shared_ptr<Security::IJwtService> jwtService,
        std::shared_ptr<Security::IPasswordHasher> passwordHasher,
        std::shared_ptr<Security::ITokenGenerator> tokenGenerator,
        std::shared_ptr<Security::IGoogleAuthService> googleAuthService = nullptr,
        std::shared_ptr<Domain::Repositories::ICacheRepository> cacheRepo = nullptr
    );

    Domain::Common::Result<Common::ApiResponse<DTOs::RegisterResponse>> registerUser(
        const DTOs::RegisterRequest& request
    );

    Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>> login(
        const DTOs::LoginRequest& request,
        std::string_view ipAddress,
        std::string_view userAgent,
        std::string_view deviceName,
        std::string_view clientType = "browser"
    );

    Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>> loginWithGoogle(
        const DTOs::GoogleLoginRequest& request,
        std::string_view ipAddress,
        std::string_view userAgent
    );

    Domain::Common::Result<Common::ApiResponse<DTOs::GoogleAuthUrlResponse>> getGoogleAuthUrl(
        std::string_view state
    );

    Domain::Common::Result<Common::ApiResponse<void>> setPassword(
        std::string_view userId,
        const DTOs::SetPasswordRequest& request
    );

    Domain::Common::Result<Common::ApiResponse<DTOs::TokenResponse>> refresh(
        const DTOs::RefreshTokenRequest& request,
        std::string_view ipAddress,
        std::string_view userAgent
    );

    Domain::Common::Result<Common::ApiResponse<void>> logout(
        std::string_view sessionId,
        std::string_view userId,
        std::string_view ipAddress,
        std::string_view userAgent
    );

    Domain::Common::Result<Common::ApiResponse<void>> logoutAll(
        std::string_view userId,
        std::string_view ipAddress,
        std::string_view userAgent
    );

private:
    std::shared_ptr<Domain::Repositories::IUserRepository> m_userRepo;
    std::shared_ptr<Domain::Repositories::ISessionRepository> m_sessionRepo;
    std::shared_ptr<Domain::Repositories::IAuditLogRepository> m_auditRepo;
    std::shared_ptr<Security::IJwtService> m_jwtService;
    std::shared_ptr<Security::IPasswordHasher> m_passwordHasher;
    std::shared_ptr<Security::ITokenGenerator> m_tokenGenerator;
    std::shared_ptr<Security::IGoogleAuthService> m_googleAuthService;
    std::shared_ptr<Domain::Repositories::ICacheRepository> m_cacheRepo;
};

} // namespace Application::UseCases
