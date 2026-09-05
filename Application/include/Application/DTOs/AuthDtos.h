#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Application::DTOs {

struct UserDto {
    std::string id;
    std::string email;
    std::string role;
    std::string authProvider{"local"};
    std::string avatarUrl{""};
    bool googleLinked{false};
};

struct RegisterRequest {
    std::string email;
    std::string password;
    std::string role{"user"};
};

struct RegisterResponse {
    std::string id;
    std::string email;
    std::string role;
    std::string createdAt;
};

struct LoginRequest {
    std::string email;
    std::string password;
};

struct TokenResponse {
    std::string accessToken;
    std::string refreshToken;
    std::string tokenType{"Bearer"};
    int64_t expiresIn{900}; // seconds
    std::string sessionId;
    UserDto user;
};

struct RefreshTokenRequest {
    std::string refreshToken;
};

struct SessionResponse {
    std::string id;
    std::string userId;
    std::string device;
    std::string ipAddress;
    std::string jti;
    std::string createdAt;
    std::string lastSeenAt;
    std::string expiresAt;
    bool current{false};
    std::string status{"active"}; // "active", "revoked", "expired"
};

struct SessionListResponse {
    std::vector<SessionResponse> sessions;
};

struct AdminRevokeUserSessionsRequest {
    std::string reason{"Administrative mass revocation"};
};

} // namespace Application::DTOs
