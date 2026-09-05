#pragma once

#include <string>

namespace Application::DTOs {

struct GoogleLoginRequest {
    std::string idToken;
    std::string code;
    std::string codeVerifier; // RFC 7636 PKCE code_verifier
    std::string state;        // State token used to resolve cached PKCE verifier
    std::string deviceName{"Unknown Device"};
    std::string clientType{"browser"};
};

struct GoogleAuthUrlResponse {
    std::string authUrl;
    std::string state;
    std::string codeVerifier;  // Server-generated PKCE code_verifier
    std::string codeChallenge; // S256 code_challenge sent to Google
};

struct SetPasswordRequest {
    std::string password;
};

} // namespace Application::DTOs
