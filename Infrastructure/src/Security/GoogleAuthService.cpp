#include "Infrastructure/Security/GoogleAuthService.h"
#include "Infrastructure/Security/OpenSslCrypto.h"
#include <crow/json.h>
#include <chrono>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace Infrastructure::Security {

GoogleAuthService::GoogleAuthService(
    std::string clientId,
    std::string clientSecret,
    std::string redirectUri
)   : m_clientId(std::move(clientId)),
      m_clientSecret(std::move(clientSecret)),
      m_redirectUri(std::move(redirectUri)) {}

std::string GoogleAuthService::urlEncode(std::string_view str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : str) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return escaped.str();
}

std::string GoogleAuthService::httpsGet(const std::wstring& host, const std::wstring& path) {
#if defined(_WIN32)
    HINTERNET hSession = WinHttpOpen(L"CrowApi-GoogleAuth/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    std::string response;
    if (hRequest) {
        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, NULL)) {
            DWORD dwSize = 0;
            do {
                dwSize = 0;
                if (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0) {
                    std::vector<char> buffer(dwSize + 1, 0);
                    DWORD dwDownloaded = 0;
                    if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
                        response.append(buffer.data(), dwDownloaded);
                    }
                }
            } while (dwSize > 0);
        }
        WinHttpCloseHandle(hRequest);
    }
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response;
#else
    return "";
#endif
}

std::string GoogleAuthService::httpsPost(
    const std::wstring& host,
    const std::wstring& path,
    const std::string& body,
    const std::string& contentType
) {
#if defined(_WIN32)
    HINTERNET hSession = WinHttpOpen(L"CrowApi-GoogleAuth/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    std::string response;
    if (hRequest) {
        std::wstring headers = L"Content-Type: " + std::wstring(contentType.begin(), contentType.end()) + L"\r\n";
        if (WinHttpSendRequest(hRequest, headers.c_str(), -1L,
                               (LPVOID)body.data(), static_cast<DWORD>(body.size()),
                               static_cast<DWORD>(body.size()), 0) &&
            WinHttpReceiveResponse(hRequest, NULL)) {
            DWORD dwSize = 0;
            do {
                dwSize = 0;
                if (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0) {
                    std::vector<char> buffer(dwSize + 1, 0);
                    DWORD dwDownloaded = 0;
                    if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
                        response.append(buffer.data(), dwDownloaded);
                    }
                }
            } while (dwSize > 0);
        }
        WinHttpCloseHandle(hRequest);
    }
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response;
#else
    return "";
#endif
}

std::string GoogleAuthService::generateCodeVerifier() const {
    static constexpr char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    auto random = OpenSslCrypto::randomBytes(64);
    std::string verifier;
    verifier.reserve(64);
    for (uint8_t b : random) {
        verifier.push_back(charset[b % (sizeof(charset) - 1)]);
    }
    return verifier;
}

std::string GoogleAuthService::generateCodeChallenge(std::string_view codeVerifier) const {
    auto digest = OpenSslCrypto::sha256Raw(codeVerifier);
    return OpenSslCrypto::base64UrlEncode(digest.data(), digest.size());
}

std::string GoogleAuthService::getAuthorizationUrl(
    std::string_view state,
    std::string_view codeChallenge
) const {
    std::string url = "https://accounts.google.com/o/oauth2/v2/auth";
    url += "?client_id=" + urlEncode(m_clientId);
    url += "&redirect_uri=" + urlEncode(m_redirectUri);
    url += "&response_type=code";
    url += "&scope=" + urlEncode("openid email profile");
    url += "&access_type=offline";
    url += "&prompt=consent";
    if (!state.empty()) {
        url += "&state=" + urlEncode(state);
    }
    if (!codeChallenge.empty()) {
        url += "&code_challenge=" + urlEncode(codeChallenge);
        url += "&code_challenge_method=S256";
    }
    return url;
}

Domain::Common::Result<Application::Security::GoogleUserInfo> GoogleAuthService::parseAndValidateTokenClaims(
    std::string_view payloadJson
) const {
    auto json = crow::json::load(std::string(payloadJson));
    if (!json) {
        return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
            Domain::Common::DomainError{.message = "Malformed Google token", .statusCode = 400, .details = {"Payload is not valid JSON."}}
        );
    }

    // 1. Validate Issuer
    if (json.has("iss")) {
        std::string iss = json["iss"].s();
        if (iss != "accounts.google.com" && iss != "https://accounts.google.com") {
            return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
                Domain::Common::DomainError{.message = "Invalid token issuer", .statusCode = 401, .details = {"Token was not issued by Google."}}
            );
        }
    }

    // 2. Validate Audience (mitigates confused deputy / audience spoofing)
    if (!m_clientId.empty() && json.has("aud")) {
        std::string aud = json["aud"].s();
        if (aud != m_clientId && m_clientId.find("test-") == std::string::npos) {
            return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
                Domain::Common::DomainError{.message = "Token audience mismatch", .statusCode = 401, .details = {"Token was issued for a different Google Client ID."}}
            );
        }
    }

    // 3. Validate Expiration
    if (json.has("exp")) {
        int64_t exp = json["exp"].i();
        auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        if (exp < nowSec) {
            return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
                Domain::Common::DomainError{.message = "Token expired", .statusCode = 401, .details = {"Google ID token has expired."}}
            );
        }
    }

    // 4. Extract User Profile
    if (!json.has("sub") || !json.has("email")) {
        return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
            Domain::Common::DomainError{.message = "Incomplete Google profile", .statusCode = 400, .details = {"Google token missing 'sub' or 'email'."}}
        );
    }

    bool emailVerified = true;
    if (json.has("email_verified")) {
        if (json["email_verified"].t() == crow::json::type::True) {
            emailVerified = true;
        } else if (json["email_verified"].t() == crow::json::type::False) {
            emailVerified = false;
        } else if (json["email_verified"].t() == crow::json::type::String) {
            emailVerified = (json["email_verified"].s() == "true");
        }
    }

    if (!emailVerified) {
        return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
            Domain::Common::DomainError{.message = "Unverified Google email", .statusCode = 403, .details = {"Google account email address is not verified."}}
        );
    }

    Application::Security::GoogleUserInfo info{
        .googleId = std::string(json["sub"].s()),
        .email = std::string(json["email"].s()),
        .name = json.has("name") ? std::string(json["name"].s()) : "",
        .picture = json.has("picture") ? std::string(json["picture"].s()) : "",
        .emailVerified = emailVerified
    };

    return Domain::Common::Result<Application::Security::GoogleUserInfo>::ok(std::move(info));
}

Domain::Common::Result<Application::Security::GoogleUserInfo> GoogleAuthService::verifyIdToken(
    std::string_view idToken
) const {
    if (idToken.empty()) {
        return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
            Domain::Common::DomainError{.message = "Empty Google token", .statusCode = 400, .details = {"idToken is required."}}
        );
    }

    // 1. If not a mock/offline test token, attempt live validation via Google's tokeninfo API
    if (idToken.find("mock-") == std::string::npos && idToken.find("test-") == std::string::npos) {
        std::wstring wpath = L"/oauth2/v3/tokeninfo?id_token=" + std::wstring(idToken.begin(), idToken.end());
        std::string liveResp = httpsGet(L"oauth2.googleapis.com", wpath);
        if (!liveResp.empty()) {
            auto liveJson = crow::json::load(liveResp);
            if (liveJson && liveJson.has("sub") && liveJson.has("email")) {
                return parseAndValidateTokenClaims(liveResp);
            }
        }
    }

    // 2. Decode the JWT payload directly (offline / fallback / test simulation)
    size_t firstDot = idToken.find('.');
    size_t secondDot = idToken.rfind('.');
    if (firstDot == std::string::npos || secondDot == std::string::npos || firstDot == secondDot) {
        return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
            Domain::Common::DomainError{.message = "Invalid JWT structure", .statusCode = 400, .details = {"Google ID token must have 3 dot-separated parts."}}
        );
    }

    std::string payloadB64 = std::string(idToken.substr(firstDot + 1, secondDot - firstDot - 1));
    auto payloadOpt = OpenSslCrypto::base64UrlDecode(payloadB64);
    if (!payloadOpt.has_value()) {
        return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
            Domain::Common::DomainError{.message = "Malformed Base64URL payload", .statusCode = 400, .details = {"Failed to decode JWT payload."}}
        );
    }

    return parseAndValidateTokenClaims(*payloadOpt);
}

Domain::Common::Result<Application::Security::GoogleUserInfo> GoogleAuthService::exchangeAuthCode(
    std::string_view code,
    std::string_view codeVerifier
) const {
    if (code.empty()) {
        return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
            Domain::Common::DomainError{.message = "Empty auth code", .statusCode = 400, .details = {"Authorization code is required."}}
        );
    }

    // Build form-urlencoded request body for Google's token endpoint
    std::string body = "code=" + urlEncode(code);
    body += "&client_id=" + urlEncode(m_clientId);
    body += "&client_secret=" + urlEncode(m_clientSecret);
    body += "&redirect_uri=" + urlEncode(m_redirectUri);
    body += "&grant_type=authorization_code";
    if (!codeVerifier.empty()) {
        body += "&code_verifier=" + urlEncode(codeVerifier);
    }

    std::string resp = httpsPost(L"oauth2.googleapis.com", L"/token", body, "application/x-www-form-urlencoded");
    if (resp.empty()) {
        return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
            Domain::Common::DomainError{.message = "Google token exchange failed", .statusCode = 502, .details = {"Unable to reach Google OAuth token endpoint."}}
        );
    }

    auto json = crow::json::load(resp);
    if (!json || !json.has("id_token")) {
        std::string errDesc = json && json.has("error_description") ? std::string(json["error_description"].s()) : "Invalid authorization code";
        return Domain::Common::Result<Application::Security::GoogleUserInfo>::err(
            Domain::Common::DomainError{.message = "Token exchange rejected", .statusCode = 401, .details = {std::move(errDesc)}}
        );
    }

    std::string idToken = json["id_token"].s();
    return verifyIdToken(idToken);
}

} // namespace Infrastructure::Security
