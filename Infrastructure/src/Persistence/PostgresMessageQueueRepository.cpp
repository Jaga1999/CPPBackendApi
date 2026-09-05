#include "Infrastructure/Persistence/PostgresMessageQueueRepository.h"
#include <chrono>
#include <iostream>

namespace Infrastructure::Persistence {

PostgresMessageQueueRepository::PostgresMessageQueueRepository(std::shared_ptr<PostgresDb> db)
    : m_db(std::move(db)) {}

uint64_t PostgresMessageQueueRepository::publish(std::string_view topic, std::string_view payload) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            "INSERT INTO message_queue (topic, payload, status, retry_count, created_at) "
            "VALUES ($1, $2::jsonb, 'PENDING', 0, clock_timestamp()) RETURNING id",
            pqxx::params{
                std::string(topic),
                std::string(payload)
            }
        );

        uint64_t id = 0;
        if (!rows.empty()) {
            id = rows[0][0].as<uint64_t>();
        }

        // Notify topic subscribers
        tx.exec("SELECT pg_notify($1, $2)", pqxx::params{std::string(topic), std::to_string(id)});
        tx.commit();
        return id;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresMessageQueueRepository] publish error: " << ex.what() << std::endl;
        return 0;
    }
}

std::optional<Domain::Entities::QueueMessage> PostgresMessageQueueRepository::pollNext(std::string_view topic) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};

        // Atomic concurrent consumer pattern using FOR UPDATE SKIP LOCKED
        auto rows = tx.exec(
            "UPDATE message_queue "
            "SET status = 'PROCESSING', processed_at = clock_timestamp() "
            "WHERE id = ("
            "    SELECT id FROM message_queue "
            "    WHERE topic = $1 AND status = 'PENDING' "
            "    ORDER BY id ASC "
            "    FOR UPDATE SKIP LOCKED "
            "    LIMIT 1"
            ") "
            "RETURNING id, topic, payload::text, status, retry_count, created_at, processed_at",
            pqxx::params{std::string(topic)}
        );

        if (rows.empty()) {
            tx.commit();
            return std::nullopt;
        }

        const auto& row = rows[0];
        Domain::Entities::QueueMessage msg{
            .id = row[0].as<uint64_t>(),
            .topic = row[1].as<std::string>(),
            .payload = row[2].as<std::string>(),
            .status = row[3].as<std::string>(),
            .retryCount = row[4].as<int>(),
            .createdAt = std::chrono::system_clock::now(),
            .processedAt = std::chrono::system_clock::now()
        };

        tx.commit();
        return msg;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresMessageQueueRepository] pollNext error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

bool PostgresMessageQueueRepository::acknowledge(uint64_t id) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto res = tx.exec(
            "UPDATE message_queue SET status = 'COMPLETED', processed_at = clock_timestamp() WHERE id = $1",
            pqxx::params{id}
        );
        tx.commit();
        return res.affected_rows() > 0;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresMessageQueueRepository] acknowledge error: " << ex.what() << std::endl;
        return false;
    }
}

bool PostgresMessageQueueRepository::fail(uint64_t id) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto res = tx.exec(
            "UPDATE message_queue SET status = 'FAILED', retry_count = retry_count + 1 WHERE id = $1",
            pqxx::params{id}
        );
        tx.commit();
        return res.affected_rows() > 0;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresMessageQueueRepository] fail error: " << ex.what() << std::endl;
        return false;
    }
}

std::vector<Domain::Entities::QueueMetrics> PostgresMessageQueueRepository::getMetrics() {
    std::vector<Domain::Entities::QueueMetrics> metrics;
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            "SELECT topic, "
            "COUNT(*) FILTER (WHERE status = 'PENDING') AS pending, "
            "COUNT(*) FILTER (WHERE status = 'PROCESSING') AS processing, "
            "COUNT(*) FILTER (WHERE status = 'COMPLETED') AS completed, "
            "COUNT(*) FILTER (WHERE status = 'FAILED') AS failed "
            "FROM message_queue GROUP BY topic ORDER BY topic ASC"
        );

        for (const auto& r : rows) {
            Domain::Entities::QueueMetrics m{
                .topic = r[0].as<std::string>(),
                .pendingCount = r[1].as<int64_t>(),
                .processingCount = r[2].as<int64_t>(),
                .completedCount = r[3].as<int64_t>(),
                .failedCount = r[4].as<int64_t>()
            };
            metrics.push_back(std::move(m));
        }
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresMessageQueueRepository] getMetrics error: " << ex.what() << std::endl;
    }
    return metrics;
}

} // namespace Infrastructure::Persistence
