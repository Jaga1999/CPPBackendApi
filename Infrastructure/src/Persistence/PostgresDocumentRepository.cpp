#include "Infrastructure/Persistence/PostgresDocumentRepository.h"
#include <chrono>
#include <iostream>

namespace Infrastructure::Persistence {

PostgresDocumentRepository::PostgresDocumentRepository(std::shared_ptr<PostgresDb> db)
    : m_db(std::move(db)) {}

Domain::Entities::DocumentEntity PostgresDocumentRepository::insert(
    std::string_view collection,
    std::string_view jsonData
) {
    Domain::Entities::DocumentEntity doc{
        .collection = std::string(collection),
        .data = std::string(jsonData)
    };

    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            "INSERT INTO documents (collection_name, data, created_at, updated_at) "
            "VALUES ($1, $2::jsonb, clock_timestamp(), clock_timestamp()) "
            "RETURNING id::text",
            pqxx::params{
                std::string(collection),
                std::string(jsonData)
            }
        );

        if (!rows.empty()) {
            doc.id = rows[0][0].as<std::string>();
        }
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresDocumentRepository] insert error: " << ex.what() << std::endl;
    }
    return doc;
}

std::optional<Domain::Entities::DocumentEntity> PostgresDocumentRepository::findById(
    std::string_view collection,
    std::string_view id
) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            "SELECT id::text, collection_name, data::text FROM documents "
            "WHERE collection_name = $1 AND id = $2::uuid",
            pqxx::params{
                std::string(collection),
                std::string(id)
            }
        );

        if (rows.empty()) {
            tx.commit();
            return std::nullopt;
        }

        Domain::Entities::DocumentEntity doc{
            .id = rows[0][0].as<std::string>(),
            .collection = rows[0][1].as<std::string>(),
            .data = rows[0][2].as<std::string>()
        };
        tx.commit();
        return doc;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresDocumentRepository] findById error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

std::vector<Domain::Entities::DocumentEntity> PostgresDocumentRepository::queryByFilter(
    std::string_view collection,
    std::string_view jsonbFilter
) {
    std::vector<Domain::Entities::DocumentEntity> results;
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};

        std::string filterStr(jsonbFilter);
        bool hasFilter = !filterStr.empty() && filterStr != "{}" && filterStr.find_first_not_of(" \t\n\r") != std::string::npos;

        pqxx::result rows;
        if (hasFilter) {
            rows = tx.exec(
                "SELECT id::text, collection_name, data::text FROM documents "
                "WHERE collection_name = $1 AND data @> $2::jsonb "
                "ORDER BY created_at DESC LIMIT 100",
                pqxx::params{
                    std::string(collection),
                    filterStr
                }
            );
        } else {
            rows = tx.exec(
                "SELECT id::text, collection_name, data::text FROM documents "
                "WHERE collection_name = $1 "
                "ORDER BY created_at DESC LIMIT 100",
                pqxx::params{std::string(collection)}
            );
        }

        for (const auto& r : rows) {
            results.push_back(Domain::Entities::DocumentEntity{
                .id = r[0].as<std::string>(),
                .collection = r[1].as<std::string>(),
                .data = r[2].as<std::string>()
            });
        }
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresDocumentRepository] queryByFilter error: " << ex.what() << std::endl;
    }
    return results;
}

std::optional<Domain::Entities::DocumentEntity> PostgresDocumentRepository::update(
    std::string_view collection,
    std::string_view id,
    std::string_view jsonData
) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            "UPDATE documents SET data = $1::jsonb, updated_at = clock_timestamp() "
            "WHERE collection_name = $2 AND id = $3::uuid "
            "RETURNING id::text, collection_name, data::text",
            pqxx::params{
                std::string(jsonData),
                std::string(collection),
                std::string(id)
            }
        );

        if (rows.empty()) {
            tx.commit();
            return std::nullopt;
        }

        Domain::Entities::DocumentEntity doc{
            .id = rows[0][0].as<std::string>(),
            .collection = rows[0][1].as<std::string>(),
            .data = rows[0][2].as<std::string>()
        };
        tx.commit();
        return doc;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresDocumentRepository] update error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

bool PostgresDocumentRepository::remove(std::string_view collection, std::string_view id) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto res = tx.exec(
            "DELETE FROM documents WHERE collection_name = $1 AND id = $2::uuid",
            pqxx::params{
                std::string(collection),
                std::string(id)
            }
        );
        tx.commit();
        return res.affected_rows() > 0;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresDocumentRepository] remove error: " << ex.what() << std::endl;
        return false;
    }
}

} // namespace Infrastructure::Persistence
