#pragma once

#include "Domain/Entities/CacheEntry.h"
#include <optional>
#include <string_view>

namespace Domain::Repositories {

class ICacheRepository {
public:
    virtual ~ICacheRepository() = default;

    virtual std::optional<Entities::CacheEntry> get(std::string_view key) = 0;
    virtual void set(std::string_view key, std::string_view value, int ttlSeconds) = 0;
    virtual bool remove(std::string_view key) = 0;
    virtual size_t cleanupExpired() = 0;
};

} // namespace Domain::Repositories
