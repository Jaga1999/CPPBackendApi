#include "Infrastructure/Persistence/PostgresUserRepository.h"
#include <iostream>

namespace Infrastructure::Persistence {

PostgresUserRepository::PostgresUserRepository(std::shared_ptr<PostgresDb> db)
    : m_db(std::move(db)) {}

namespace {

Domain::Entities::User mapUserRow(const auto& r) {
    Domain::Entities::User user{
        .id = r[0].as<std::string>(),
        .email = r[1].as<std::string>(),
        .passwordHash = r[2].is_null() ? std::nullopt : std::make_optional(r[2].as<std::string>()),
        .role = r[3].as<std::string>(),
        .isActive = r[4].as<bool>(),
        .failedLoginAttempts = r[5].as<int>()
    };
    if (!r[7].is_null()) {
        user.googleId = r[7].as<std::string>();
    }
    if (!r[8].is_null()) {
        user.authProvider = r[8].as<std::string>();
    }
    if (!r[9].is_null()) {
        user.avatarUrl = r[9].as<std::string>();
    }
    return user;
}

const char* USER_SELECT_FIELDS =
    "SELECT id::text, email, password_hash, role, is_active, failed_login_attempts, locked_until, google_id, auth_provider, avatar_url FROM users ";

} // anonymous namespace

std::optional<Domain::Entities::User> PostgresUserRepository::findById(std::string_view id) const {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            std::string(USER_SELECT_FIELDS) + "WHERE id = $1::uuid",
            pqxx::params{std::string(id)}
        );

        if (rows.empty()) {
            tx.commit();
            return std::nullopt;
        }

        auto user = mapUserRow(rows[0]);
        tx.commit();
        return user;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresUserRepository] findById error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Domain::Entities::User> PostgresUserRepository::findByEmail(std::string_view email) const {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            std::string(USER_SELECT_FIELDS) + "WHERE email = $1",
            pqxx::params{std::string(email)}
        );

        if (rows.empty()) {
            tx.commit();
            return std::nullopt;
        }

        auto user = mapUserRow(rows[0]);
        tx.commit();
        return user;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresUserRepository] findByEmail error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Domain::Entities::User> PostgresUserRepository::findByGoogleId(std::string_view googleId) const {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            std::string(USER_SELECT_FIELDS) + "WHERE google_id = $1",
            pqxx::params{std::string(googleId)}
        );

        if (rows.empty()) {
            tx.commit();
            return std::nullopt;
        }

        auto user = mapUserRow(rows[0]);
        tx.commit();
        return user;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresUserRepository] findByGoogleId error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

Domain::Entities::User PostgresUserRepository::create(Domain::Entities::User user) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        std::string prov = user.authProvider.empty() ? "local" : user.authProvider;
        auto rows = tx.exec(
            "INSERT INTO users (email, password_hash, role, is_active, failed_login_attempts, google_id, auth_provider, avatar_url, created_at, updated_at) "
            "VALUES ($1, $2, $3, $4, 0, $5, $6, $7, clock_timestamp(), clock_timestamp()) "
            "RETURNING id::text",
            pqxx::params{
                user.email,
                user.passwordHash,
                user.role,
                user.isActive,
                user.googleId,
                prov,
                user.avatarUrl
            }
        );

        if (!rows.empty()) {
            user.id = rows[0][0].as<std::string>();
        }
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresUserRepository] create error: " << ex.what() << std::endl;
    }
    return user;
}

bool PostgresUserRepository::linkGoogleAccount(std::string_view userId, std::string_view googleId, std::string_view avatarUrl) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        std::optional<std::string> av = avatarUrl.empty() ? std::nullopt : std::make_optional(std::string(avatarUrl));
        auto res = tx.exec(
            "UPDATE users SET google_id = $1, "
            "auth_provider = CASE WHEN password_hash IS NOT NULL AND password_hash != '' THEN 'local+google' ELSE 'google' END, "
            "avatar_url = COALESCE($2, avatar_url), "
            "updated_at = clock_timestamp() "
            "WHERE id = $3::uuid",
            pqxx::params{
                std::string(googleId),
                av,
                std::string(userId)
            }
        );
        tx.commit();
        return res.affected_rows() > 0;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresUserRepository] linkGoogleAccount error: " << ex.what() << std::endl;
        return false;
    }
}

bool PostgresUserRepository::setPassword(std::string_view userId, std::string_view newPasswordHash) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto res = tx.exec(
            "UPDATE users SET password_hash = $1, "
            "auth_provider = CASE WHEN google_id IS NOT NULL AND google_id != '' THEN 'local+google' ELSE 'local' END, "
            "updated_at = clock_timestamp() "
            "WHERE id = $2::uuid",
            pqxx::params{
                std::string(newPasswordHash),
                std::string(userId)
            }
        );
        tx.commit();
        return res.affected_rows() > 0;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresUserRepository] setPassword error: " << ex.what() << std::endl;
        return false;
    }
}

bool PostgresUserRepository::update(const Domain::Entities::User& user) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto res = tx.exec(
            "UPDATE users SET role = $1, is_active = $2, updated_at = clock_timestamp() WHERE id = $3::uuid",
            pqxx::params{
                user.role,
                user.isActive,
                user.id
            }
        );
        tx.commit();
        return res.affected_rows() > 0;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresUserRepository] update error: " << ex.what() << std::endl;
        return false;
    }
}

void PostgresUserRepository::incrementFailedAttempts(std::string_view userId) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        tx.exec(
            "UPDATE users SET failed_login_attempts = failed_login_attempts + 1, updated_at = clock_timestamp() "
            "WHERE id = $1::uuid",
            pqxx::params{std::string(userId)}
        );
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresUserRepository] incrementFailedAttempts error: " << ex.what() << std::endl;
    }
}

void PostgresUserRepository::resetFailedAttempts(std::string_view userId) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        tx.exec(
            "UPDATE users SET failed_login_attempts = 0, locked_until = NULL, updated_at = clock_timestamp() "
            "WHERE id = $1::uuid",
            pqxx::params{std::string(userId)}
        );
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresUserRepository] resetFailedAttempts error: " << ex.what() << std::endl;
    }
}

void PostgresUserRepository::lockAccount(std::string_view userId, std::chrono::system_clock::time_point until) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        const auto untilSec = std::chrono::duration_cast<std::chrono::seconds>(until.time_since_epoch()).count();
        tx.exec(
            "UPDATE users SET locked_until = to_timestamp($1), updated_at = clock_timestamp() WHERE id = $2::uuid",
            pqxx::params{
                untilSec,
                std::string(userId)
            }
        );
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresUserRepository] lockAccount error: " << ex.what() << std::endl;
    }
}

} // namespace Infrastructure::Persistence
