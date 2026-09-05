#pragma once

#include "Domain/Repositories/ISessionRepository.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include <memory>

namespace Infrastructure::Persistence {

class PostgresSessionRepository : public Domain::Repositories::ISessionRepository {
public:
    explicit PostgresSessionRepository(std::shared_ptr<PostgresDb> db);
    ~PostgresSessionRepository() override = default;

    Domain::Entities::Session createSession(Domain::Entities::Session session) override;
    std::optional<Domain::Entities::Session> findById(std::string_view sessionId) const override;
    std::optional<Domain::Entities::Session> findByJti(std::string_view jti) const override;
    std::optional<Domain::Entities::Session> findByRefreshTokenHash(std::string_view hash) override;
    std::vector<Domain::Entities::Session> findUserSessions(std::string_view userId, bool includeRevoked) const override;
    std::vector<Domain::Entities::Session> findAllSessions(
        int limit,
        int offset,
        std::optional<std::string_view> userId = std::nullopt,
        std::optional<std::string_view> status = std::nullopt,
        std::optional<std::string_view> ipAddress = std::nullopt
    ) const override;

    bool updateRefreshToken(
        std::string_view sessionId,
        std::string_view newHash,
        std::string_view newJti,
        std::chrono::system_clock::time_point newExpiresAt
    ) override;

    bool revokeSession(std::string_view sessionId, std::string_view reason) override;
    size_t revokeAllUserSessions(std::string_view userId, std::string_view reason) override;

    bool isTokenRevoked(std::string_view jti) const override;
    void recordTokenRevocation(std::string_view jti, std::string_view sessionId, std::chrono::system_clock::time_point expiresAt) override;
    size_t cleanupExpiredRevocations(std::chrono::system_clock::time_point now) override;

private:
    std::shared_ptr<PostgresDb> m_db;
};

} // namespace Infrastructure::Persistence
