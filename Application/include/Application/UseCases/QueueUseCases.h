#pragma once

#include "Application/Common/ApiResponse.h"
#include "Application/DTOs/QueueDtos.h"
#include "Domain/Common/Result.h"
#include "Domain/Repositories/IMessageQueueRepository.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Application::UseCases {

class QueueUseCases {
public:
    explicit QueueUseCases(std::shared_ptr<Domain::Repositories::IMessageQueueRepository> repository);

    Domain::Common::Result<Common::ApiResponse<DTOs::QueueMessageResponse>> publish(
        DTOs::PublishMessageRequest request
    );

    Domain::Common::Result<Common::ApiResponse<DTOs::QueueMessageResponse>> poll(
        DTOs::PollMessageRequest request
    );

    Domain::Common::Result<Common::ApiResponse<void>> acknowledge(uint64_t id);
    Domain::Common::Result<Common::ApiResponse<void>> fail(uint64_t id);
    Common::ApiResponse<std::vector<DTOs::QueueMetricsResponse>> getMetrics();

private:
    std::shared_ptr<Domain::Repositories::IMessageQueueRepository> m_repository;
};

} // namespace Application::UseCases
