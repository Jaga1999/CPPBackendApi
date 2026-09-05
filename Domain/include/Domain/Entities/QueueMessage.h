#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <optional>
#include <string>

namespace Domain::Entities {

/**
 * @brief Represents an event/message in the PostgreSQL queue (Kafka alternative).
 */
struct QueueMessage {
    uint64_t id{0};
    std::string topic;
    std::string payload; // JSON representation
    std::string status{"PENDING"}; // PENDING, PROCESSING, COMPLETED, FAILED
    int retryCount{0};
    std::chrono::system_clock::time_point createdAt{std::chrono::system_clock::now()};
    std::optional<std::chrono::system_clock::time_point> processedAt{std::nullopt};

    auto operator<=>(const QueueMessage&) const = default;
};

struct QueueMetrics {
    std::string topic;
    int64_t pendingCount{0};
    int64_t processingCount{0};
    int64_t completedCount{0};
    int64_t failedCount{0};
};

} // namespace Domain::Entities
