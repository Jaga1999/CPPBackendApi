#pragma once

#include "Domain/Repositories/ITodoRepository.h"
#include <atomic>
#include <shared_mutex>
#include <vector>

namespace Infrastructure::Persistence {

class InMemoryTodoRepository : public Domain::Repositories::ITodoRepository {
public:
    InMemoryTodoRepository();
    ~InMemoryTodoRepository() override = default;

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
    mutable std::shared_mutex m_mutex;
    std::vector<Domain::Entities::Todo> m_todos;
    std::atomic<uint64_t> m_nextId{1};
};

} // namespace Infrastructure::Persistence
