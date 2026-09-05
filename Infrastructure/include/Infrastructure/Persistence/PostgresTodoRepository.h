#pragma once

#include "Domain/Repositories/ITodoRepository.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include <memory>
#include <vector>

namespace Infrastructure::Persistence {

class PostgresTodoRepository : public Domain::Repositories::ITodoRepository {
public:
    explicit PostgresTodoRepository(std::shared_ptr<PostgresDb> db);
    ~PostgresTodoRepository() override = default;

    std::vector<Domain::Entities::Todo> findAll() const override;
    std::optional<Domain::Entities::Todo> findById(uint64_t id) const override;
    Domain::Entities::Todo save(Domain::Entities::Todo todo) override;
    std::optional<Domain::Entities::Todo> update(
        uint64_t id,
        std::optional<std::string_view> title,
        std::optional<std::string_view> description,
        std::optional<bool> completed
    ) override;
    bool remove(uint64_t id) override;

private:
    std::shared_ptr<PostgresDb> m_db;
};

} // namespace Infrastructure::Persistence
