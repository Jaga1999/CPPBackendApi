#include "Infrastructure/Persistence/InMemoryTodoRepository.h"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <ranges>

namespace Infrastructure::Persistence {

InMemoryTodoRepository::InMemoryTodoRepository() {
    const auto now = std::chrono::system_clock::now();
    m_todos.push_back(Domain::Entities::Todo{
        .id = m_nextId++,
        .title = "Explore Crow C++ Framework",
        .description = "Investigate Crow capabilities and routing with C++20",
        .completed = true,
        .createdAt = now,
        .updatedAt = now
    });
    m_todos.push_back(Domain::Entities::Todo{
        .id = m_nextId++,
        .title = "Multi-Module Clean Architecture",
        .description = "Organize solution into Domain, Application, Infrastructure, Presentation, Core projects",
        .completed = false,
        .createdAt = now,
        .updatedAt = now
    });
}

std::vector<Domain::Entities::Todo> InMemoryTodoRepository::findAll() const {
    std::shared_lock lock(m_mutex);
    return m_todos;
}

std::optional<Domain::Entities::Todo> InMemoryTodoRepository::findById(uint64_t id) const {
    std::shared_lock lock(m_mutex);
    auto it = std::ranges::find_if(m_todos, [id](const auto& item) {
        return item.id == id;
    });

    if (it != m_todos.end()) {
        return *it;
    }
    return std::nullopt;
}

Domain::Entities::Todo InMemoryTodoRepository::save(Domain::Entities::Todo todo) {
    std::unique_lock lock(m_mutex);
    todo.id = m_nextId++;
    const auto now = std::chrono::system_clock::now();
    todo.createdAt = now;
    todo.updatedAt = now;
    m_todos.push_back(todo);
    return todo;
}

std::optional<Domain::Entities::Todo> InMemoryTodoRepository::update(
    uint64_t id,
    std::optional<std::string_view> title,
    std::optional<std::string_view> description,
    std::optional<bool> completed
) {
    std::unique_lock lock(m_mutex);
    auto it = std::ranges::find_if(m_todos, [id](const auto& item) {
        return item.id == id;
    });

    if (it == m_todos.end()) {
        return std::nullopt;
    }

    if (title.has_value()) {
        it->title = std::string(*title);
    }
    if (description.has_value()) {
        it->description = std::string(*description);
    }
    if (completed.has_value()) {
        it->completed = *completed;
    }

    it->updatedAt = std::chrono::system_clock::now();
    return *it;
}

bool InMemoryTodoRepository::remove(uint64_t id) {
    std::unique_lock lock(m_mutex);
    auto it = std::ranges::find_if(m_todos, [id](const auto& item) {
        return item.id == id;
    });

    if (it == m_todos.end()) {
        return false;
    }

    m_todos.erase(it);
    return true;
}

} // namespace Infrastructure::Persistence
