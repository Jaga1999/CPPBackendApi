#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <string>
#include <vector>

namespace Infrastructure::Persistence {

class PostgresDb : public std::enable_shared_from_this<PostgresDb> {
public:
    explicit PostgresDb(std::string connStr = "", size_t maxPoolSize = 20);
    ~PostgresDb();

    std::shared_ptr<pqxx::connection> getConnection();
    bool testConnection();

    static std::string getDefaultConnectionString();

private:
    void returnConnection(pqxx::connection* conn);

    std::string m_connStr;
    size_t m_maxPoolSize{20};
    size_t m_createdCount{0};
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<std::unique_ptr<pqxx::connection>> m_pool;
};

} // namespace Infrastructure::Persistence
