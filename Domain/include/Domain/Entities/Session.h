#pragma once

#include <chrono>
#include <compare>
#include <optional>
#include <string>

namespace Domain::Entities {

struct Session {
    std::string id;
    std::string userId;
    std::string jti;
    std::string refreshTokenHash; // SHA-256 hex string
    std::chrono::system_clock::time_point createdAt{std::chrono::system_clock::now()};
    std::chrono::system_clock::time_point lastSeenAt{std::chrono::system_clock::now()};
    std::chrono::system_clock::time_point expiresAt{std::chrono::system_clock::now()};
    std::optional<std::chrono::system_clock::time_point> revokedAt{std::nullopt};
    std::optional<std::string> revocationReason{std::nullopt};
    std::string ipAddress;
    std::string userAgent;
    std::string deviceName;
    std::string clientType{"browser"};

    [[nodiscard]] bool isRevoked() const noexcept {
        return revokedAt.has_value();
    }

    [[nodiscard]] bool isExpired(std::chrono::system_clock::time_point now = std::chrono::system_clock::now()) const noexcept {
        return now >= expiresAt;
    }

    [[nodiscard]] bool isActive(std::chrono::system_clock::time_point now = std::chrono::system_clock::now()) const noexcept {
        return !isRevoked() && !isExpired(now);
    }

    auto operator<=>(const Session&) const = default;
};

} // namespace Domain::Entities
