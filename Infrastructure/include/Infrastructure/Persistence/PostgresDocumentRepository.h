#pragma once

#include "Domain/Repositories/IDocumentRepository.h"
#include "Infrastructure/Persistence/PostgresDb.h"
#include <memory>

namespace Infrastructure::Persistence {

class PostgresDocumentRepository : public Domain::Repositories::IDocumentRepository {
public:
    explicit PostgresDocumentRepository(std::shared_ptr<PostgresDb> db);
    ~PostgresDocumentRepository() override = default;

    Domain::Entities::DocumentEntity insert(std::string_view collection, std::string_view jsonData) override;
    std::optional<Domain::Entities::DocumentEntity> findById(std::string_view collection, std::string_view id) override;
    std::vector<Domain::Entities::DocumentEntity> queryByFilter(std::string_view collection, std::string_view jsonbFilter) override;
    std::optional<Domain::Entities::DocumentEntity> update(std::string_view collection, std::string_view id, std::string_view jsonData) override;
    bool remove(std::string_view collection, std::string_view id) override;

private:
    std::shared_ptr<PostgresDb> m_db;
};

} // namespace Infrastructure::Persistence
