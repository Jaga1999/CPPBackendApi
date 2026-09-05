#pragma once

#include "Domain/Entities/DocumentEntity.h"
#include <optional>
#include <string_view>
#include <vector>

namespace Domain::Repositories {

class IDocumentRepository {
public:
    virtual ~IDocumentRepository() = default;

    virtual Entities::DocumentEntity insert(std::string_view collection, std::string_view jsonData) = 0;
    virtual std::optional<Entities::DocumentEntity> findById(std::string_view collection, std::string_view id) = 0;
    virtual std::vector<Entities::DocumentEntity> queryByFilter(std::string_view collection, std::string_view jsonbFilter) = 0;
    virtual std::optional<Entities::DocumentEntity> update(std::string_view collection, std::string_view id, std::string_view jsonData) = 0;
    virtual bool remove(std::string_view collection, std::string_view id) = 0;
};

} // namespace Domain::Repositories
