#pragma once

#include "Domain/Common/Result.h"
#include <chrono>
#include <compare>
#include <cstdint>
#include <string>
#include <string_view>

namespace Domain::Entities {

struct Todo {
    uint64_t id{0};
    std::string title;
    std::string description;
    bool completed{false};
    std::chrono::system_clock::time_point createdAt{std::chrono::system_clock::now()};
    std::chrono::system_clock::time_point updatedAt{std::chrono::system_clock::now()};

    // C++20 three-way comparison
    auto operator<=>(const Todo& other) const = default;

    // Business validation rule
    static Common::Result<void, Common::DomainError> validate(std::string_view title);
};

} // namespace Domain::Entities
