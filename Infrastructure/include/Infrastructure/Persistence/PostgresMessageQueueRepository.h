#pragma once

#include "Domain/Repositories/IMessageQueueRepository.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include <memory>

namespace Infrastructure::Persistence {

class PostgresMessageQueueRepository : public Domain::Repositories::IMessageQueueRepository {
public:
    explicit PostgresMessageQueueRepository(std::shared_ptr<PostgresDb> db);
    ~PostgresMessageQueueRepository() override = default;

    uint64_t publish(std::string_view topic, std::string_view payload) override;
    std::optional<Domain::Entities::QueueMessage> pollNext(std::string_view topic) override;
    bool acknowledge(uint64_t id) override;
    bool fail(uint64_t id) override;
    std::vector<Domain::Entities::QueueMetrics> getMetrics() override;

private:
    std::shared_ptr<PostgresDb> m_db;
};

} // namespace Infrastructure::Persistence
