#pragma once

#include "Application/Security/IGoogleAuthService.h"
#include <string>
#include <string_view>

namespace Infrastructure::Security {

class GoogleAuthService : public Application::Security::IGoogleAuthService {
public:
    explicit GoogleAuthService(
        std::string clientId,
        std::string clientSecret,
        std::string redirectUri
    );

    [[nodiscard]] std::string getAuthorizationUrl(
        std::string_view state,
        std::string_view codeChallenge = ""
    ) const override;
    [[nodiscard]] Domain::Common::Result<Application::Security::GoogleUserInfo> verifyIdToken(std::string_view idToken) const override;
    [[nodiscard]] Domain::Common::Result<Application::Security::GoogleUserInfo> exchangeAuthCode(
        std::string_view code,
        std::string_view codeVerifier = ""
    ) const override;

    [[nodiscard]] std::string generateCodeVerifier() const override;
    [[nodiscard]] std::string generateCodeChallenge(std::string_view codeVerifier) const override;

    [[nodiscard]] const std::string& getClientId() const noexcept { return m_clientId; }
    [[nodiscard]] const std::string& getRedirectUri() const noexcept { return m_redirectUri; }

private:
    std::string m_clientId;
    std::string m_clientSecret;
    std::string m_redirectUri;

    Domain::Common::Result<Application::Security::GoogleUserInfo> parseAndValidateTokenClaims(
        std::string_view payloadJson
    ) const;

    static std::string urlEncode(std::string_view str);
    static std::string httpsGet(const std::wstring& host, const std::wstring& path);
    static std::string httpsPost(const std::wstring& host, const std::wstring& path, const std::string& body, const std::string& contentType);
};

} // namespace Infrastructure::Security
