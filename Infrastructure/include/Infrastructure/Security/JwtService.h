#pragma once

#include "Application/Security/IJwtService.h"
#include "Application/Security/IKeyManager.h"
#include <memory>
#include <string>

namespace Infrastructure::Security {

class JwtService : public Application::Security::IJwtService {
public:
    explicit JwtService(
        std::shared_ptr<Application::Security::IKeyManager> keyManager,
        std::string issuer = "crowapi-auth-service",
        std::string audience = "crowapi-api"
    );

    std::string createAccessToken(
        std::string_view userId,
        std::string_view role,
        std::string_view sessionId,
        std::string_view jti,
        std::chrono::seconds ttl = std::chrono::seconds(900)
    ) override;

    Domain::Common::Result<Application::Security::JwtClaims> validateAccessToken(
        std::string_view token
    ) const override;

    const std::string& getIssuer() const noexcept { return m_issuer; }
    const std::string& getAudience() const noexcept { return m_audience; }

private:
    std::shared_ptr<Application::Security::IKeyManager> m_keyManager;
    std::string m_issuer;
    std::string m_audience;
};

} // namespace Infrastructure::Security
