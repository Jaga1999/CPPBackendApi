#pragma once

#include "Domain/Entities/QueueMessage.h"
#include <chrono>
#include <cstdint>
#include <format>
#include <optional>
#include <string>

namespace Application::DTOs {

struct PublishMessageRequest {
    std::string topic;
    std::string payload; // JSON payload
};

struct PollMessageRequest {
    std::string topic;
};

struct QueueMessageResponse {
    uint64_t id{0};
    std::string topic;
    std::string payload;
    std::string status;
    int retryCount{0};
    std::string createdAt;
    std::optional<std::string> processedAt{std::nullopt};

    static QueueMessageResponse fromDomain(const Domain::Entities::QueueMessage& msg) {
        auto formatTimestamp = [](const std::chrono::system_clock::time_point& tp) -> std::string {
            try {
                return std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(tp));
            } catch (...) {
                return "1970-01-01T00:00:00Z";
            }
        };

        std::optional<std::string> procOpt = std::nullopt;
        if (msg.processedAt.has_value()) {
            procOpt = formatTimestamp(*msg.processedAt);
        }

        return QueueMessageResponse{
            .id = msg.id,
            .topic = msg.topic,
            .payload = msg.payload,
            .status = msg.status,
            .retryCount = msg.retryCount,
            .createdAt = formatTimestamp(msg.createdAt),
            .processedAt = procOpt
        };
    }
};

struct QueueMetricsResponse {
    std::string topic;
    int64_t pending{0};
    int64_t processing{0};
    int64_t completed{0};
    int64_t failed{0};

    static QueueMetricsResponse fromDomain(const Domain::Entities::QueueMetrics& m) {
        return QueueMetricsResponse{
            .topic = m.topic,
            .pending = m.pendingCount,
            .processing = m.processingCount,
            .completed = m.completedCount,
            .failed = m.failedCount
        };
    }
};

} // namespace Application::DTOs
