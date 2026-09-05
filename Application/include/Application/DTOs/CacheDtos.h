#pragma once

#include "Domain/Entities/CacheEntry.h"
#include <chrono>
#include <format>
#include <string>

namespace Application::DTOs {

struct SetCacheRequest {
    std::string key;
    std::string value;
    int ttlSeconds{3600};
};

struct CacheResponse {
    std::string key;
    std::string value;
    int ttlSeconds{3600};
    std::string createdAt;
    std::string expiresAt;
    bool isExpired{false};

    static CacheResponse fromDomain(const Domain::Entities::CacheEntry& entry) {
        auto formatTimestamp = [](const std::chrono::system_clock::time_point& tp) -> std::string {
            try {
                return std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(tp));
            } catch (...) {
                return "1970-01-01T00:00:00Z";
            }
        };

        return CacheResponse{
            .key = entry.key,
            .value = entry.value,
            .ttlSeconds = entry.ttlSeconds,
            .createdAt = formatTimestamp(entry.createdAt),
            .expiresAt = formatTimestamp(entry.expiresAt),
            .isExpired = entry.isExpired()
        };
    }
};

} // namespace Application::DTOs
