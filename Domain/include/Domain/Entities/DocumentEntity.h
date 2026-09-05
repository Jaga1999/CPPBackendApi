#pragma once

#include <chrono>
#include <compare>
#include <string>

namespace Domain::Entities {

/**
 * @brief Represents a schemaless JSONB Document (MongoDB alternative in PostgreSQL).
 */
struct DocumentEntity {
    std::string id; // UUID string
    std::string collection;
    std::string data; // JSONB text
    std::chrono::system_clock::time_point createdAt{std::chrono::system_clock::now()};
    std::chrono::system_clock::time_point updatedAt{std::chrono::system_clock::now()};

    auto operator<=>(const DocumentEntity&) const = default;
};

} // namespace Domain::Entities
