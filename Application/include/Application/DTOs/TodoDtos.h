#pragma once

#include "Domain/Entities/Todo.h"
#include <compare>
#include <cstdint>
#include <format>
#include <optional>
#include <string>

namespace Application::DTOs {

struct CreateTodoRequest {
    std::string title;
    std::string description;
};

struct UpdateTodoRequest {
    std::optional<std::string> title;
    std::optional<std::string> description;
    std::optional<bool> completed;
};

struct TodoResponse {
    uint64_t id{0};
    std::string title;
    std::string description;
    bool completed{false};
    std::string createdAt;
    std::string updatedAt;

    auto operator<=>(const TodoResponse&) const = default;

    static TodoResponse fromDomain(const Domain::Entities::Todo& todo) {
        auto formatTimestamp = [](const std::chrono::system_clock::time_point& tp) -> std::string {
            try {
                return std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(tp));
            } catch (...) {
                return "1970-01-01T00:00:00Z";
            }
        };

        return TodoResponse{
            .id = todo.id,
            .title = todo.title,
            .description = todo.description,
            .completed = todo.completed,
            .createdAt = formatTimestamp(todo.createdAt),
            .updatedAt = formatTimestamp(todo.updatedAt)
        };
    }
};

} // namespace Application::DTOs
