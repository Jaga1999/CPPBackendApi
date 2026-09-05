#pragma once

#include "Domain/Repositories/IAuditLogRepository.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include <memory>

namespace Infrastructure::Persistence {

class PostgresAuditLogRepository : public Domain::Repositories::IAuditLogRepository {
public:
    explicit PostgresAuditLogRepository(std::shared_ptr<PostgresDb> db);
    ~PostgresAuditLogRepository() override = default;

    void record(const Domain::Entities::AuditLog& log) override;
    std::vector<Domain::Entities::AuditLog> findLogs(
        std::optional<std::string_view> userId,
        std::optional<std::string_view> eventType,
        int limit,
        int offset
    ) const override;

private:
    std::shared_ptr<PostgresDb> m_db;
};

} // namespace Infrastructure::Persistence
