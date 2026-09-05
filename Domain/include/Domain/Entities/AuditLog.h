#pragma once

#include <chrono>
#include <compare>
#include <optional>
#include <string>

namespace Domain::Entities {

struct AuditLog {
    uint64_t id{0};
    std::string eventType; // LOGIN_SUCCESS, REFRESH_REUSE_DETECTED, etc.
    std::optional<std::string> userId{std::nullopt};
    std::optional<std::string> adminUserId{std::nullopt};
    std::optional<std::string> sessionId{std::nullopt};
    std::string ipAddress;
    std::string userAgent;
    std::string reason;
    std::chrono::system_clock::time_point createdAt{std::chrono::system_clock::now()};

    auto operator<=>(const AuditLog&) const = default;
};

} // namespace Domain::Entities
