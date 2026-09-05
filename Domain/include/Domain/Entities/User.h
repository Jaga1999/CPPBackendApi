#pragma once

#include <chrono>
#include <compare>
#include <optional>
#include <string>

namespace Domain::Entities {

struct User {
    std::string id;
    std::string email;
    std::optional<std::string> passwordHash{std::nullopt};
    std::string role{"user"}; // "user", "admin"
    bool isActive{true};
    int failedLoginAttempts{0};
    std::optional<std::chrono::system_clock::time_point> lockedUntil{std::nullopt};
    std::chrono::system_clock::time_point createdAt{std::chrono::system_clock::now()};
    std::chrono::system_clock::time_point updatedAt{std::chrono::system_clock::now()};
    std::optional<std::string> googleId{std::nullopt};
    std::string authProvider{"local"}; // "local", "google", "local+google"
    std::optional<std::string> avatarUrl{std::nullopt};

    auto operator<=>(const User&) const = default;
};

} // namespace Domain::Entities
