#include "Infrastructure/Persistence/PostgresSessionRepository.h"
#include <chrono>
#include <iostream>

namespace Infrastructure::Persistence {

namespace {

std::chrono::system_clock::time_point parseTimestamp(int64_t epochSec) {
    return std::chrono::system_clock::time_point{std::chrono::seconds(epochSec)};
}

int64_t toEpoch(std::chrono::system_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}

} // anonymous namespace

PostgresSessionRepository::PostgresSessionRepository(std::shared_ptr<PostgresDb> db)
    : m_db(std::move(db)) {}

Domain::Entities::Session PostgresSessionRepository::createSession(Domain::Entities::Session session) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};

        int64_t expSec = toEpoch(session.expiresAt);
        auto rows = tx.exec(
            "INSERT INTO sessions (user_id, jti, refresh_token_hash, created_at, last_seen_at, expires_at, ip_address, user_agent, device_name, client_type) "
            "VALUES ($1::uuid, $2, $3, clock_timestamp(), clock_timestamp(), to_timestamp($4), $5, $6, $7, $8) "
            "RETURNING id::text",
            pqxx::params{
                session.userId,
                session.jti,
                session.refreshTokenHash,
                expSec,
                session.ipAddress,
                session.userAgent,
                session.deviceName,
                session.clientType
            }
        );

        if (!rows.empty()) {
            session.id = rows[0][0].as<std::string>();
        }
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] createSession error: " << ex.what() << std::endl;
    }
    return session;
}

std::optional<Domain::Entities::Session> PostgresSessionRepository::findById(std::string_view sessionId) const {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            "SELECT id::text, user_id::text, jti, refresh_token_hash, "
            "extract(epoch from created_at)::bigint, extract(epoch from last_seen_at)::bigint, extract(epoch from expires_at)::bigint, "
            "extract(epoch from revoked_at)::bigint, revocation_reason, ip_address, user_agent, device_name, client_type "
            "FROM sessions WHERE (id::text = $1 OR jti = $1)",
            pqxx::params{std::string(sessionId)}
        );

        if (rows.empty()) {
            tx.commit();
            return std::nullopt;
        }

        const auto& r = rows[0];
        Domain::Entities::Session s{
            .id = r[0].as<std::string>(),
            .userId = r[1].as<std::string>(),
            .jti = r[2].as<std::string>(),
            .refreshTokenHash = r[3].as<std::string>(),
            .createdAt = parseTimestamp(r[4].as<int64_t>()),
            .lastSeenAt = parseTimestamp(r[5].as<int64_t>()),
            .expiresAt = parseTimestamp(r[6].as<int64_t>()),
            .revokedAt = r[7].is_null() ? std::nullopt : std::make_optional(parseTimestamp(r[7].as<int64_t>())),
            .revocationReason = r[8].is_null() ? std::nullopt : std::make_optional(r[8].as<std::string>()),
            .ipAddress = r[9].is_null() ? "" : r[9].as<std::string>(),
            .userAgent = r[10].is_null() ? "" : r[10].as<std::string>(),
            .deviceName = r[11].is_null() ? "" : r[11].as<std::string>(),
            .clientType = r[12].is_null() ? "browser" : r[12].as<std::string>()
        };

        tx.commit();
        return s;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] findById error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Domain::Entities::Session> PostgresSessionRepository::findByJti(std::string_view jti) const {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            "SELECT id::text, user_id::text, jti, refresh_token_hash, "
            "extract(epoch from created_at)::bigint, extract(epoch from last_seen_at)::bigint, extract(epoch from expires_at)::bigint, "
            "extract(epoch from revoked_at)::bigint, revocation_reason, ip_address, user_agent, device_name, client_type "
            "FROM sessions WHERE jti = $1",
            pqxx::params{std::string(jti)}
        );

        if (rows.empty()) {
            tx.commit();
            return std::nullopt;
        }

        const auto& r = rows[0];
        Domain::Entities::Session s{
            .id = r[0].as<std::string>(),
            .userId = r[1].as<std::string>(),
            .jti = r[2].as<std::string>(),
            .refreshTokenHash = r[3].as<std::string>(),
            .createdAt = parseTimestamp(r[4].as<int64_t>()),
            .lastSeenAt = parseTimestamp(r[5].as<int64_t>()),
            .expiresAt = parseTimestamp(r[6].as<int64_t>()),
            .revokedAt = r[7].is_null() ? std::nullopt : std::make_optional(parseTimestamp(r[7].as<int64_t>())),
            .revocationReason = r[8].is_null() ? std::nullopt : std::make_optional(r[8].as<std::string>()),
            .ipAddress = r[9].is_null() ? "" : r[9].as<std::string>(),
            .userAgent = r[10].is_null() ? "" : r[10].as<std::string>(),
            .deviceName = r[11].is_null() ? "" : r[11].as<std::string>(),
            .clientType = r[12].is_null() ? "browser" : r[12].as<std::string>()
        };

        tx.commit();
        return s;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] findByJti error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Domain::Entities::Session> PostgresSessionRepository::findByRefreshTokenHash(std::string_view hash) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        // Lock row to prevent concurrent refresh races
        auto rows = tx.exec(
            "SELECT id::text, user_id::text, jti, refresh_token_hash, "
            "extract(epoch from created_at)::bigint, extract(epoch from last_seen_at)::bigint, extract(epoch from expires_at)::bigint, "
            "extract(epoch from revoked_at)::bigint, revocation_reason, ip_address, user_agent, device_name, client_type "
            "FROM sessions WHERE refresh_token_hash = $1 FOR UPDATE",
            pqxx::params{std::string(hash)}
        );

        if (rows.empty()) {
            // Check if this hash was previously rotated (Reuse Detection!)
            auto reuseRows = tx.exec(
                "SELECT id::text, user_id::text, jti, refresh_token_hash, "
                "extract(epoch from created_at)::bigint, extract(epoch from last_seen_at)::bigint, extract(epoch from expires_at)::bigint, "
                "extract(epoch from revoked_at)::bigint, revocation_reason, ip_address, user_agent, device_name, client_type "
                "FROM sessions WHERE previous_refresh_token_hash = $1",
                pqxx::params{std::string(hash)}
            );
            if (!reuseRows.empty()) {
                const auto& r = reuseRows[0];
                Domain::Entities::Session s{
                    .id = r[0].as<std::string>(),
                    .userId = r[1].as<std::string>(),
                    .jti = r[2].as<std::string>(),
                    .refreshTokenHash = r[3].as<std::string>(),
                    .createdAt = parseTimestamp(r[4].as<int64_t>()),
                    .lastSeenAt = parseTimestamp(r[5].as<int64_t>()),
                    .expiresAt = parseTimestamp(r[6].as<int64_t>()),
                    .revokedAt = std::chrono::system_clock::now(),
                    .revocationReason = "TOKEN_REUSE_DETECTED",
                    .ipAddress = r[9].is_null() ? "" : r[9].as<std::string>(),
                    .userAgent = r[10].is_null() ? "" : r[10].as<std::string>(),
                    .deviceName = r[11].is_null() ? "" : r[11].as<std::string>(),
                    .clientType = r[12].is_null() ? "browser" : r[12].as<std::string>()
                };
                tx.commit();
                return s;
            }

            tx.commit();
            return std::nullopt;
        }

        const auto& r = rows[0];
        Domain::Entities::Session s{
            .id = r[0].as<std::string>(),
            .userId = r[1].as<std::string>(),
            .jti = r[2].as<std::string>(),
            .refreshTokenHash = r[3].as<std::string>(),
            .createdAt = parseTimestamp(r[4].as<int64_t>()),
            .lastSeenAt = parseTimestamp(r[5].as<int64_t>()),
            .expiresAt = parseTimestamp(r[6].as<int64_t>()),
            .revokedAt = r[7].is_null() ? std::nullopt : std::make_optional(parseTimestamp(r[7].as<int64_t>())),
            .revocationReason = r[8].is_null() ? std::nullopt : std::make_optional(r[8].as<std::string>()),
            .ipAddress = r[9].is_null() ? "" : r[9].as<std::string>(),
            .userAgent = r[10].is_null() ? "" : r[10].as<std::string>(),
            .deviceName = r[11].is_null() ? "" : r[11].as<std::string>(),
            .clientType = r[12].is_null() ? "browser" : r[12].as<std::string>()
        };

        tx.commit();
        return s;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] findByRefreshTokenHash error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<Domain::Entities::Session> PostgresSessionRepository::findUserSessions(
    std::string_view userId,
    bool includeRevoked
) const {
    std::vector<Domain::Entities::Session> list;
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};

        std::string query =
            "SELECT id::text, user_id::text, jti, refresh_token_hash, "
            "extract(epoch from created_at)::bigint, extract(epoch from last_seen_at)::bigint, extract(epoch from expires_at)::bigint, "
            "extract(epoch from revoked_at)::bigint, revocation_reason, ip_address, user_agent, device_name, client_type "
            "FROM sessions WHERE user_id = $1::uuid ";
        if (!includeRevoked) {
            query += "AND revoked_at IS NULL AND expires_at > clock_timestamp() ";
        }
        query += "ORDER BY created_at DESC";

        auto rows = tx.exec(query, pqxx::params{std::string(userId)});
        for (const auto& r : rows) {
            list.push_back(Domain::Entities::Session{
                .id = r[0].as<std::string>(),
                .userId = r[1].as<std::string>(),
                .jti = r[2].as<std::string>(),
                .refreshTokenHash = r[3].as<std::string>(),
                .createdAt = parseTimestamp(r[4].as<int64_t>()),
                .lastSeenAt = parseTimestamp(r[5].as<int64_t>()),
                .expiresAt = parseTimestamp(r[6].as<int64_t>()),
                .revokedAt = r[7].is_null() ? std::nullopt : std::make_optional(parseTimestamp(r[7].as<int64_t>())),
                .revocationReason = r[8].is_null() ? std::nullopt : std::make_optional(r[8].as<std::string>()),
                .ipAddress = r[9].is_null() ? "" : r[9].as<std::string>(),
                .userAgent = r[10].is_null() ? "" : r[10].as<std::string>(),
                .deviceName = r[11].is_null() ? "" : r[11].as<std::string>(),
                .clientType = r[12].is_null() ? "browser" : r[12].as<std::string>()
            });
        }
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] findUserSessions error: " << ex.what() << std::endl;
    }
    return list;
}

std::vector<Domain::Entities::Session> PostgresSessionRepository::findAllSessions(
    int limit,
    int offset,
    std::optional<std::string_view> userId,
    std::optional<std::string_view> status,
    std::optional<std::string_view> ipAddress
) const {
    std::vector<Domain::Entities::Session> list;
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};

        std::string query =
            "SELECT id::text, user_id::text, jti, refresh_token_hash, "
            "extract(epoch from created_at)::bigint, extract(epoch from last_seen_at)::bigint, extract(epoch from expires_at)::bigint, "
            "extract(epoch from revoked_at)::bigint, revocation_reason, ip_address, user_agent, device_name, client_type "
            "FROM sessions WHERE 1=1 ";

        std::vector<std::string> params;
        int paramIdx = 1;

        if (userId.has_value() && !userId->empty()) {
            query += "AND user_id = $" + std::to_string(paramIdx++) + "::uuid ";
            params.push_back(std::string(*userId));
        }
        if (status.has_value()) {
            if (*status == "active") {
                query += "AND revoked_at IS NULL AND expires_at > clock_timestamp() ";
            } else if (*status == "revoked") {
                query += "AND revoked_at IS NOT NULL ";
            } else if (*status == "expired") {
                query += "AND expires_at <= clock_timestamp() ";
            }
        }
        if (ipAddress.has_value() && !ipAddress->empty()) {
            query += "AND ip_address = $" + std::to_string(paramIdx++) + " ";
            params.push_back(std::string(*ipAddress));
        }

        query += "ORDER BY created_at DESC LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

        pqxx::result rows;
        if (params.empty()) {
            rows = tx.exec(query);
        } else if (params.size() == 1) {
            rows = tx.exec(query, pqxx::params{params[0]});
        } else if (params.size() == 2) {
            rows = tx.exec(query, pqxx::params{params[0], params[1]});
        }

        for (const auto& r : rows) {
            list.push_back(Domain::Entities::Session{
                .id = r[0].as<std::string>(),
                .userId = r[1].as<std::string>(),
                .jti = r[2].as<std::string>(),
                .refreshTokenHash = r[3].as<std::string>(),
                .createdAt = parseTimestamp(r[4].as<int64_t>()),
                .lastSeenAt = parseTimestamp(r[5].as<int64_t>()),
                .expiresAt = parseTimestamp(r[6].as<int64_t>()),
                .revokedAt = r[7].is_null() ? std::nullopt : std::make_optional(parseTimestamp(r[7].as<int64_t>())),
                .revocationReason = r[8].is_null() ? std::nullopt : std::make_optional(r[8].as<std::string>()),
                .ipAddress = r[9].is_null() ? "" : r[9].as<std::string>(),
                .userAgent = r[10].is_null() ? "" : r[10].as<std::string>(),
                .deviceName = r[11].is_null() ? "" : r[11].as<std::string>(),
                .clientType = r[12].is_null() ? "browser" : r[12].as<std::string>()
            });
        }
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] findAllSessions error: " << ex.what() << std::endl;
    }
    return list;
}

bool PostgresSessionRepository::updateRefreshToken(
    std::string_view sessionId,
    std::string_view newHash,
    std::string_view newJti,
    std::chrono::system_clock::time_point newExpiresAt
) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        int64_t expSec = toEpoch(newExpiresAt);

        auto res = tx.exec(
            "UPDATE sessions "
            "SET previous_refresh_token_hash = refresh_token_hash, "
            "    refresh_token_hash = $1, jti = $2, expires_at = to_timestamp($3), last_seen_at = clock_timestamp() "
            "WHERE (id::text = $4 OR jti = $4) AND revoked_at IS NULL",
            pqxx::params{
                std::string(newHash),
                std::string(newJti),
                expSec,
                std::string(sessionId)
            }
        );
        tx.commit();
        return res.affected_rows() > 0;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] updateRefreshToken error: " << ex.what() << std::endl;
        return false;
    }
}

bool PostgresSessionRepository::revokeSession(std::string_view sessionId, std::string_view reason) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};

        auto rows = tx.exec(
            "UPDATE sessions "
            "SET revoked_at = clock_timestamp(), revocation_reason = $1 "
            "WHERE (id::text = $2 OR jti = $2) AND revoked_at IS NULL "
            "RETURNING id::text, jti, extract(epoch from expires_at)::bigint",
            pqxx::params{
                std::string(reason),
                std::string(sessionId)
            }
        );

        if (rows.empty()) {
            tx.commit();
            return false;
        }

        std::string actualSid = rows[0][0].as<std::string>();
        std::string jti = rows[0][1].as<std::string>();
        int64_t expSec = rows[0][2].as<int64_t>();

        // Insert into fast token_revocations table
        tx.exec(
            "INSERT INTO token_revocations (jti, session_id, expires_at, revoked_at) "
            "VALUES ($1, $2::uuid, to_timestamp($3), clock_timestamp()) "
            "ON CONFLICT (jti) DO NOTHING",
            pqxx::params{
                jti,
                actualSid,
                expSec
            }
        );

        // Notify other instances
        tx.exec("SELECT pg_notify('session_revoked', $1)", pqxx::params{actualSid});
        tx.commit();
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] revokeSession error: " << ex.what() << std::endl;
        return false;
    }
}

size_t PostgresSessionRepository::revokeAllUserSessions(std::string_view userId, std::string_view reason) {
    size_t count = 0;
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};

        auto rows = tx.exec(
            "UPDATE sessions "
            "SET revoked_at = clock_timestamp(), revocation_reason = $1 "
            "WHERE user_id = $2::uuid AND revoked_at IS NULL "
            "RETURNING id::text, jti, extract(epoch from expires_at)::bigint",
            pqxx::params{
                std::string(reason),
                std::string(userId)
            }
        );

        count = rows.size();
        for (const auto& r : rows) {
            std::string sid = r[0].as<std::string>();
            std::string jti = r[1].as<std::string>();
            int64_t expSec = r[2].as<int64_t>();

            tx.exec(
                "INSERT INTO token_revocations (jti, session_id, expires_at, revoked_at) "
                "VALUES ($1, $2::uuid, to_timestamp($3), clock_timestamp()) "
                "ON CONFLICT (jti) DO NOTHING",
                pqxx::params{
                    jti,
                    sid,
                    expSec
                }
            );
            tx.exec("SELECT pg_notify('session_revoked', $1)", pqxx::params{sid});
        }

        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] revokeAllUserSessions error: " << ex.what() << std::endl;
    }
    return count;
}

bool PostgresSessionRepository::isTokenRevoked(std::string_view jti) const {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};

        auto rows = tx.exec("SELECT 1 FROM token_revocations WHERE jti = $1", pqxx::params{std::string(jti)});
        if (!rows.empty()) {
            tx.commit();
            return true;
        }

        // Also fallback check sessions table
        auto sessRows = tx.exec(
            "SELECT 1 FROM sessions WHERE jti = $1 AND revoked_at IS NOT NULL",
            pqxx::params{std::string(jti)}
        );
        tx.commit();
        return !sessRows.empty();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] isTokenRevoked error: " << ex.what() << std::endl;
        return false;
    }
}

void PostgresSessionRepository::recordTokenRevocation(
    std::string_view jti,
    std::string_view sessionId,
    std::chrono::system_clock::time_point expiresAt
) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        int64_t expSec = toEpoch(expiresAt);

        tx.exec(
            "INSERT INTO token_revocations (jti, session_id, expires_at, revoked_at) "
            "VALUES ($1, $2::uuid, to_timestamp($3), clock_timestamp()) "
            "ON CONFLICT (jti) DO NOTHING",
            pqxx::params{
                std::string(jti),
                std::string(sessionId),
                expSec
            }
        );
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] recordTokenRevocation error: " << ex.what() << std::endl;
    }
}

size_t PostgresSessionRepository::cleanupExpiredRevocations(std::chrono::system_clock::time_point /*now*/) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto res = tx.exec("DELETE FROM token_revocations WHERE expires_at <= clock_timestamp()");
        tx.commit();
        return static_cast<size_t>(res.affected_rows());
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresSessionRepository] cleanupExpiredRevocations error: " << ex.what() << std::endl;
        return 0;
    }
}

} // namespace Infrastructure::Persistence
