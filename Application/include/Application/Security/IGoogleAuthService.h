#pragma once

#include "Domain/Common/Result.h"
#include <string>
#include <string_view>

namespace Application::Security {

struct GoogleUserInfo {
    std::string googleId;     // Google unique sub ID
    std::string email;        // Verified email
    std::string name;         // Display name
    std::string picture;      // Avatar URL
    bool emailVerified{false};
};

class IGoogleAuthService {
public:
    virtual ~IGoogleAuthService() = default;

    [[nodiscard]] virtual std::string getAuthorizationUrl(
        std::string_view state,
        std::string_view codeChallenge = ""
    ) const = 0;

    [[nodiscard]] virtual Domain::Common::Result<GoogleUserInfo> verifyIdToken(std::string_view idToken) const = 0;

    [[nodiscard]] virtual Domain::Common::Result<GoogleUserInfo> exchangeAuthCode(
        std::string_view code,
        std::string_view codeVerifier = ""
    ) const = 0;

    [[nodiscard]] virtual std::string generateCodeVerifier() const = 0;
    [[nodiscard]] virtual std::string generateCodeChallenge(std::string_view codeVerifier) const = 0;
};

} // namespace Application::Security
