#pragma once

#include "Domain/Entities/User.h"
#include <chrono>
#include <optional>
#include <string_view>

namespace Domain::Repositories {

class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    virtual std::optional<Entities::User> findById(std::string_view id) const = 0;
    virtual std::optional<Entities::User> findByEmail(std::string_view email) const = 0;
    virtual std::optional<Entities::User> findByGoogleId(std::string_view googleId) const = 0;
    virtual Entities::User create(Entities::User user) = 0;
    virtual bool update(const Entities::User& user) = 0;
    virtual bool linkGoogleAccount(std::string_view userId, std::string_view googleId, std::string_view avatarUrl) = 0;
    virtual bool setPassword(std::string_view userId, std::string_view newPasswordHash) = 0;
    virtual void incrementFailedAttempts(std::string_view userId) = 0;
    virtual void resetFailedAttempts(std::string_view userId) = 0;
    virtual void lockAccount(std::string_view userId, std::chrono::system_clock::time_point until) = 0;
};

} // namespace Domain::Repositories
