#pragma once

#include "Domain/Repositories/ICacheRepository.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include <memory>

namespace Infrastructure::Persistence {

class PostgresCacheRepository : public Domain::Repositories::ICacheRepository {
public:
    explicit PostgresCacheRepository(std::shared_ptr<PostgresDb> db);
    ~PostgresCacheRepository() override = default;

    std::optional<Domain::Entities::CacheEntry> get(std::string_view key) override;
    void set(std::string_view key, std::string_view value, int ttlSeconds) override;
    bool remove(std::string_view key) override;
    size_t cleanupExpired() override;

private:
    std::shared_ptr<PostgresDb> m_db;
};

} // namespace Infrastructure::Persistence
