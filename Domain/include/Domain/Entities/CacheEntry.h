#pragma once

#include <chrono>
#include <compare>
#include <string>

namespace Domain::Entities {

/**
 * @brief Represents a Key-Value cache entry (Redis alternative in PostgreSQL).
 */
struct CacheEntry {
    std::string key;
    std::string value;
    int ttlSeconds{3600};
    std::chrono::system_clock::time_point createdAt{std::chrono::system_clock::now()};
    std::chrono::system_clock::time_point expiresAt{std::chrono::system_clock::now() + std::chrono::seconds(3600)};

    auto operator<=>(const CacheEntry&) const = default;

    [[nodiscard]] bool isExpired() const noexcept {
        return std::chrono::system_clock::now() > expiresAt;
    }
};

} // namespace Domain::Entities
