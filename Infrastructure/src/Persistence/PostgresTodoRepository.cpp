#include "Infrastructure/Persistence/PostgresTodoRepository.h"
#include <chrono>
#include <iostream>

namespace Infrastructure::Persistence {

PostgresTodoRepository::PostgresTodoRepository(std::shared_ptr<PostgresDb> db)
    : m_db(std::move(db)) {}

std::vector<Domain::Entities::Todo> PostgresTodoRepository::findAll() const {
    std::vector<Domain::Entities::Todo> list;
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec("SELECT id, title, COALESCE(description, ''), completed, created_at, updated_at FROM todos ORDER BY id ASC");

        for (const auto& row : rows) {
            Domain::Entities::Todo item{
                .id = row[0].as<uint64_t>(),
                .title = row[1].as<std::string>(),
                .description = row[2].as<std::string>(),
                .completed = row[3].as<bool>()
            };
            list.push_back(std::move(item));
        }
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresTodoRepository] findAll error: " << ex.what() << std::endl;
    }
    return list;
}

std::optional<Domain::Entities::Todo> PostgresTodoRepository::findById(uint64_t id) const {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec("SELECT id, title, COALESCE(description, ''), completed, created_at, updated_at FROM todos WHERE id = $1", pqxx::params{id});
        if (rows.empty()) {
            return std::nullopt;
        }

        const auto& row = rows[0];
        Domain::Entities::Todo item{
            .id = row[0].as<uint64_t>(),
            .title = row[1].as<std::string>(),
            .description = row[2].as<std::string>(),
            .completed = row[3].as<bool>()
        };
        tx.commit();
        return item;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresTodoRepository] findById error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

Domain::Entities::Todo PostgresTodoRepository::save(Domain::Entities::Todo todo) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto rows = tx.exec(
            "INSERT INTO todos (title, description, completed, created_at, updated_at) "
            "VALUES ($1, $2, $3, clock_timestamp(), clock_timestamp()) RETURNING id",
            pqxx::params{todo.title, todo.description, todo.completed}
        );
        if (!rows.empty()) {
            todo.id = rows[0][0].as<uint64_t>();
        }
        tx.commit();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresTodoRepository] save error: " << ex.what() << std::endl;
    }
    return todo;
}

std::optional<Domain::Entities::Todo> PostgresTodoRepository::update(
    uint64_t id,
    std::optional<std::string_view> title,
    std::optional<std::string_view> description,
    std::optional<bool> completed
) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};

        auto existing = tx.exec("SELECT title, COALESCE(description, ''), completed FROM todos WHERE id = $1 FOR UPDATE", pqxx::params{id});
        if (existing.empty()) {
            return std::nullopt;
        }

        std::string newTitle = title.has_value() ? std::string(*title) : existing[0][0].as<std::string>();
        std::string newDesc = description.has_value() ? std::string(*description) : existing[0][1].as<std::string>();
        bool newCompleted = completed.has_value() ? *completed : existing[0][2].as<bool>();

        tx.exec(
            "UPDATE todos SET title = $1, description = $2, completed = $3, updated_at = clock_timestamp() WHERE id = $4",
            pqxx::params{newTitle, newDesc, newCompleted, id}
        );

        tx.commit();

        return Domain::Entities::Todo{
            .id = id,
            .title = std::move(newTitle),
            .description = std::move(newDesc),
            .completed = newCompleted,
            .createdAt = std::chrono::system_clock::now(),
            .updatedAt = std::chrono::system_clock::now()
        };
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresTodoRepository] update error: " << ex.what() << std::endl;
        return std::nullopt;
    }
}

bool PostgresTodoRepository::remove(uint64_t id) {
    try {
        auto conn = m_db->getConnection();
        pqxx::work tx{*conn};
        auto res = tx.exec("DELETE FROM todos WHERE id = $1", pqxx::params{id});
        tx.commit();
        return res.affected_rows() > 0;
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresTodoRepository] remove error: " << ex.what() << std::endl;
        return false;
    }
}

} // namespace Infrastructure::Persistence
