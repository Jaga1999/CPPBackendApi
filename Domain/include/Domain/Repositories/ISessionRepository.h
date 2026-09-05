#pragma once

#include "Domain/Entities/Session.h"
#include <chrono>
#include <optional>
#include <string_view>
#include <vector>

namespace Domain::Repositories {

class ISessionRepository {
public:
    virtual ~ISessionRepository() = default;

    virtual Entities::Session createSession(Entities::Session session) = 0;
    virtual std::optional<Entities::Session> findById(std::string_view sessionId) const = 0;
    virtual std::optional<Entities::Session> findByJti(std::string_view jti) const = 0;
    virtual std::optional<Entities::Session> findByRefreshTokenHash(std::string_view hash) = 0;
    virtual std::vector<Entities::Session> findUserSessions(std::string_view userId, bool includeRevoked) const = 0;
    virtual std::vector<Entities::Session> findAllSessions(
        int limit,
        int offset,
        std::optional<std::string_view> userId = std::nullopt,
        std::optional<std::string_view> status = std::nullopt,
        std::optional<std::string_view> ipAddress = std::nullopt
    ) const = 0;

    virtual bool updateRefreshToken(
        std::string_view sessionId,
        std::string_view newHash,
        std::string_view newJti,
        std::chrono::system_clock::time_point newExpiresAt
    ) = 0;

    virtual bool revokeSession(std::string_view sessionId, std::string_view reason) = 0;
    virtual size_t revokeAllUserSessions(std::string_view userId, std::string_view reason) = 0;

    // Token Revocation cache / denylist methods
    virtual bool isTokenRevoked(std::string_view jti) const = 0;
    virtual void recordTokenRevocation(std::string_view jti, std::string_view sessionId, std::chrono::system_clock::time_point expiresAt) = 0;
    virtual size_t cleanupExpiredRevocations(std::chrono::system_clock::time_point now) = 0;
};

} // namespace Domain::Repositories
