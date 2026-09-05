#pragma once

#include "Domain/Entities/AuditLog.h"
#include <optional>
#include <string_view>
#include <vector>

namespace Domain::Repositories {

class IAuditLogRepository {
public:
    virtual ~IAuditLogRepository() = default;

    virtual void record(const Entities::AuditLog& log) = 0;
    virtual std::vector<Entities::AuditLog> findLogs(
        std::optional<std::string_view> userId,
        std::optional<std::string_view> eventType,
        int limit,
        int offset
    ) const = 0;
};

} // namespace Domain::Repositories
