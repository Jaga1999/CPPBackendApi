#include "Infrastructure/Persistence/PostgresCacheRepository.h"
#include <chrono>
#include <iostream>

namespace Infrastructure::Persistence {

PostgresCacheRepository::PostgresCacheRepository(std::shared_ptr<PostgresDb> db)
    : m_db(std::move(db)) {}

std::optional<Domain::Entities::CacheEntry> PostgresCacheRepository::get(std::string_view key) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            "SELECT cache_key, cache_value, ttl_seconds, created_at, expires_at "
            "FROM cache_store WHERE cache_key = $1 AND expires_at > clock_timestamp()",
            pqxx::params{std::string(key)}
        );

        if (rows.empty()) {
            tx.commit();
            return std::nullopt;
        }

        const auto& row = rows[0];
        Domain::Entities::CacheEntry entry{
            .key = row[0].as<std::string>(),
            .value = row[1].as<std::string>(),
            .ttlSeconds = row[2].as<int>(),
            .createdAt = std::chrono::system_clock::now(),
            .expiresAt = std::chrono::system_clock::now() + std::chrono::seconds(row[2].as<int>())
        };

        tx.commit();
        return entry;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresCacheRepository] get error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

void PostgresCacheRepository::set(std::string_view key, std::string_view value, int ttlSeconds) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        std::string intervalStr = std::to_string(ttlSeconds) + " seconds";

        tx.exec(
            "INSERT INTO cache_store (cache_key, cache_value, ttl_seconds, created_at, expires_at) "
            "VALUES ($1, $2, $3, clock_timestamp(), clock_timestamp() + $4::interval) "
            "ON CONFLICT (cache_key) DO UPDATE SET "
            "cache_value = EXCLUDED.cache_value, "
            "ttl_seconds = EXCLUDED.ttl_seconds, "
            "expires_at = clock_timestamp() + (EXCLUDED.ttl_seconds * interval '1 second')",
            pqxx::params{
                std::string(key),
                std::string(value),
                ttlSeconds,
                intervalStr
            }
        );

        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresCacheRepository] set error: " << ex.what() << std::endl;
    }
}

bool PostgresCacheRepository::remove(std::string_view key) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto res = tx.exec("DELETE FROM cache_store WHERE cache_key = $1", pqxx::params{std::string(key)});
        tx.commit();
        return res.affected_rows() > 0;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresCacheRepository] remove error: " << ex.what() << std::endl;
        return false;
    }
}

size_t PostgresCacheRepository::cleanupExpired() {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto res = tx.exec("DELETE FROM cache_store WHERE expires_at <= clock_timestamp()");
        tx.commit();
        return static_cast<size_t>(res.affected_rows());
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresCacheRepository] cleanupExpired error: " << ex.what() << std::endl;
        return 0;
    }
}

} // namespace Infrastructure::Persistence
