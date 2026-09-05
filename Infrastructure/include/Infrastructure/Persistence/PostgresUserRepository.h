#pragma once

#include "Domain/Repositories/IUserRepository.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include <memory>

namespace Infrastructure::Persistence {

class PostgresUserRepository : public Domain::Repositories::IUserRepository {
public:
    explicit PostgresUserRepository(std::shared_ptr<PostgresDb> db);
    ~PostgresUserRepository() override = default;

    std::optional<Domain::Entities::User> findById(std::string_view id) const override;
    std::optional<Domain::Entities::User> findByEmail(std::string_view email) const override;
    std::optional<Domain::Entities::User> findByGoogleId(std::string_view googleId) const override;
    Domain::Entities::User create(Domain::Entities::User user) override;
    bool update(const Domain::Entities::User& user) override;
    bool linkGoogleAccount(std::string_view userId, std::string_view googleId, std::string_view avatarUrl) override;
    bool setPassword(std::string_view userId, std::string_view newPasswordHash) override;
    void incrementFailedAttempts(std::string_view userId) override;
    void resetFailedAttempts(std::string_view userId) override;
    void lockAccount(std::string_view userId, std::chrono::system_clock::time_point until) override;

private:
    std::shared_ptr<PostgresDb> m_db;
};

} // namespace Infrastructure::Persistence
