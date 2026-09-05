#include "Infrastructure/Persistence/PostgresAuditLogRepository.h"
#include <iostream>

namespace Infrastructure::Persistence {

PostgresAuditLogRepository::PostgresAuditLogRepository(std::shared_ptr<PostgresDb> db)
    : m_db(std::move(db)) {}

void PostgresAuditLogRepository::record(const Domain::Entities::AuditLog& log) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};

        tx.exec(
            "INSERT INTO audit_logs (event_type, user_id, admin_user_id, session_id, ip_address, user_agent, reason, created_at) "
            "VALUES ($1, $2::uuid, $3::uuid, $4::uuid, $5, $6, $7, clock_timestamp())",
            pqxx::params{
                log.eventType,
                log.userId.has_value() ? std::make_optional(*log.userId) : std::nullopt,
                log.adminUserId.has_value() ? std::make_optional(*log.adminUserId) : std::nullopt,
                log.sessionId.has_value() ? std::make_optional(*log.sessionId) : std::nullopt,
                log.ipAddress,
                log.userAgent,
                log.reason
            }
        );
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresAuditLogRepository] record error: " << ex.what() << std::endl;
    }
}

std::vector<Domain::Entities::AuditLog> PostgresAuditLogRepository::findLogs(
    std::optional<std::string_view> userId,
    std::optional<std::string_view> eventType,
    int limit,
    int offset
) const {
    std::vector<Domain::Entities::AuditLog> list;
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};

        std::string query =
            "SELECT id, event_type, user_id::text, admin_user_id::text, session_id::text, ip_address, user_agent, reason, "
            "extract(epoch from created_at)::bigint "
            "FROM audit_logs WHERE 1=1 ";

        std::vector<std::string> params;
        int paramIdx = 1;
        if (userId.has_value() && !userId->empty()) {
            query += "AND user_id = $" + std::to_string(paramIdx++) + "::uuid ";
            params.push_back(std::string(*userId));
        }
        if (eventType.has_value() && !eventType->empty()) {
            query += "AND event_type = $" + std::to_string(paramIdx++) + " ";
            params.push_back(std::string(*eventType));
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
            list.push_back(Domain::Entities::AuditLog{
                .id = r[0].as<uint64_t>(),
                .eventType = r[1].as<std::string>(),
                .userId = r[2].is_null() ? std::nullopt : std::make_optional(r[2].as<std::string>()),
                .adminUserId = r[3].is_null() ? std::nullopt : std::make_optional(r[3].as<std::string>()),
                .sessionId = r[4].is_null() ? std::nullopt : std::make_optional(r[4].as<std::string>()),
                .ipAddress = r[5].is_null() ? "" : r[5].as<std::string>(),
                .userAgent = r[6].is_null() ? "" : r[6].as<std::string>(),
                .reason = r[7].is_null() ? "" : r[7].as<std::string>(),
                .createdAt = std::chrono::system_clock::time_point{std::chrono::seconds(r[8].as<int64_t>())}
            });
        }
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresAuditLogRepository] findLogs error: " << ex.what() << std::endl;
    }
    return list;
}

} // namespace Infrastructure::Persistence
