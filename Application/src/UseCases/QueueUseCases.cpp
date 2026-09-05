#include "Application/UseCases/QueueUseCases.h"
#include <format>

namespace Application::UseCases {

QueueUseCases::QueueUseCases(std::shared_ptr<Domain::Repositories::IMessageQueueRepository> repository)
    : m_repository(std::move(repository)) {}

Domain::Common::Result<Common::ApiResponse<DTOs::QueueMessageResponse>> QueueUseCases::publish(
    DTOs::PublishMessageRequest request
) {
    if (request.topic.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::QueueMessageResponse>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed for queue publish",
                .statusCode = 400,
                .details = {"Field 'topic' is required and cannot be empty."}
            }
        );
    }

    if (request.payload.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::QueueMessageResponse>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed for queue publish",
                .statusCode = 400,
                .details = {"Field 'payload' is required and cannot be empty."}
            }
        );
    }

    uint64_t msgId = m_repository->publish(request.topic, request.payload);

    DTOs::QueueMessageResponse response{
        .id = msgId,
        .topic = request.topic,
        .payload = request.payload,
        .status = "PENDING",
        .retryCount = 0,
        .createdAt = Common::currentTimestampIso8601(),
        .processedAt = std::nullopt
    };

    return Domain::Common::Result<Common::ApiResponse<DTOs::QueueMessageResponse>>::ok(
        Common::ApiResponse<DTOs::QueueMessageResponse>::ok(
            std::move(response),
            std::format("Message published to topic '{}' with ID {}", request.topic, msgId),
            201
        )
    );
}

Domain::Common::Result<Common::ApiResponse<DTOs::QueueMessageResponse>> QueueUseCases::poll(
    DTOs::PollMessageRequest request
) {
    if (request.topic.empty()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::QueueMessageResponse>>::err(
            Domain::Common::DomainError{
                .message = "Validation failed for queue poll",
                .statusCode = 400,
                .details = {"Field 'topic' is required."}
            }
        );
    }

    auto msgOpt = m_repository->pollNext(request.topic);
    if (!msgOpt.has_value()) {
        return Domain::Common::Result<Common::ApiResponse<DTOs::QueueMessageResponse>>::err(
            Domain::Common::DomainError{
                .message = std::format("No pending messages in topic '{}'", request.topic),
                .statusCode = 404,
                .details = {std::format("Queue topic '{}' is empty or all messages are locked.", request.topic)}
            }
        );
    }

    auto response = DTOs::QueueMessageResponse::fromDomain(*msgOpt);
    return Domain::Common::Result<Common::ApiResponse<DTOs::QueueMessageResponse>>::ok(
        Common::ApiResponse<DTOs::QueueMessageResponse>::ok(
            std::move(response),
            std::format("Message {} locked and retrieved via FOR UPDATE SKIP LOCKED", response.id),
            200
        )
    );
}

Domain::Common::Result<Common::ApiResponse<void>> QueueUseCases::acknowledge(uint64_t id) {
    if (id == 0) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{
                .message = "Invalid message ID",
                .statusCode = 400,
                .details = {"ID must be a positive non-zero integer."}
            }
        );
    }

    bool acked = m_repository->acknowledge(id);
    if (!acked) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{
                .message = std::format("Message {} not found or already acknowledged", id),
                .statusCode = 404,
                .details = {std::format("Cannot acknowledge message ID: {}", id)}
            }
        );
    }

    return Domain::Common::Result<Common::ApiResponse<void>>::ok(
        Common::ApiResponse<void>::ok(
            std::format("Message {} acknowledged (COMPLETED)", id),
            200
        )
    );
}

Domain::Common::Result<Common::ApiResponse<void>> QueueUseCases::fail(uint64_t id) {
    if (id == 0) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{
                .message = "Invalid message ID",
                .statusCode = 400,
                .details = {"ID must be a positive non-zero integer."}
            }
        );
    }

    bool failed = m_repository->fail(id);
    if (!failed) {
        return Domain::Common::Result<Common::ApiResponse<void>>::err(
            Domain::Common::DomainError{
                .message = std::format("Message {} not found", id),
                .statusCode = 404,
                .details = {std::format("Cannot fail message ID: {}", id)}
            }
        );
    }

    return Domain::Common::Result<Common::ApiResponse<void>>::ok(
        Common::ApiResponse<void>::ok(
            std::format("Message {} marked as FAILED", id),
            200
        )
    );
}

Common::ApiResponse<std::vector<DTOs::QueueMetricsResponse>> QueueUseCases::getMetrics() {
    auto domainMetrics = m_repository->getMetrics();
    std::vector<DTOs::QueueMetricsResponse> responses;
    responses.reserve(domainMetrics.size());
    for (const auto& m : domainMetrics) {
        responses.push_back(DTOs::QueueMetricsResponse::fromDomain(m));
    }

    return Common::ApiResponse<std::vector<DTOs::QueueMetricsResponse>>::ok(
        std::move(responses),
        "Queue metrics retrieved successfully",
        200
    );
}

} // namespace Application::UseCases
