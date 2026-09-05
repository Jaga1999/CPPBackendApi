#pragma once

#include "Domain/Entities/QueueMessage.h"
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace Domain::Repositories {

class IMessageQueueRepository {
public:
    virtual ~IMessageQueueRepository() = default;

    virtual uint64_t publish(std::string_view topic, std::string_view payload) = 0;
    virtual std::optional<Entities::QueueMessage> pollNext(std::string_view topic) = 0;
    virtual bool acknowledge(uint64_t id) = 0;
    virtual bool fail(uint64_t id) = 0;
    virtual std::vector<Entities::QueueMetrics> getMetrics() = 0;
};

} // namespace Domain::Repositories
