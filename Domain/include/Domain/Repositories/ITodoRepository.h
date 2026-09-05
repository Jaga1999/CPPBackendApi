#pragma once

#include "Domain/Entities/Todo.h"
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace Domain::Repositories {

class ITodoRepository {
public:
    virtual ~ITodoRepository() = default;

    virtual std::vector<Entities::Todo> findAll() const = 0;
    virtual std::optional<Entities::Todo> findById(uint64_t id) const = 0;
    virtual Entities::Todo save(Entities::Todo todo) = 0;
    virtual std::optional<Entities::Todo> update(
        uint64_t id,
        std::optional<std::string_view> title,
        std::optional<std::string_view> description,
        std::optional<bool> completed
    ) = 0;
    virtual bool remove(uint64_t id) = 0;
};

} // namespace Domain::Repositories
