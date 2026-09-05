#pragma once

#include "Domain/Common/Result.h"
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace Application::Security {

struct JwtClaims {
    std::string iss;
    std::string sub; // User ID
    std::string aud;
    int64_t iat{0};
    int64_t nbf{0};
    int64_t exp{0};
    std::string jti; // Token ID
    std::string sid; // Session ID
    std::string role;
    std::string kid; // Key ID from JWS Header
};

class IJwtService {
public:
    virtual ~IJwtService() = default;

    virtual std::string createAccessToken(
        std::string_view userId,
        std::string_view role,
        std::string_view sessionId,
        std::string_view jti,
        std::chrono::seconds ttl = std::chrono::seconds(900)
    ) = 0;

    virtual Domain::Common::Result<JwtClaims> validateAccessToken(std::string_view token) const = 0;
};

} // namespace Application::Security
