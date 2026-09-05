#include "Infrastructure/Persistence/PostgresDb.h"
#include <cstdlib>
#include <iostream>

namespace Infrastructure::Persistence {

namespace {

std::string getEnvSafe(const char* name) {
#if defined(_MSC_VER)
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, name) == 0 && buf != nullptr) {
        std::string res(buf);
        free(buf);
        return res;
    }
    return "";
#else
    const char* val = std::getenv(name);
    return val ? std::string(val) : "";
#endif
}

} // anonymous namespace

std::string PostgresDb::getDefaultConnectionString() {
    std::string envUrl = getEnvSafe("DATABASE_URL");
    if (!envUrl.empty()) {
        return envUrl;
    }
    std::string envConn = getEnvSafe("POSTGRES_CONN_STR");
    if (!envConn.empty()) {
        return envConn;
    }
    std::string host = getEnvSafe("DB_HOST");
    std::string port = getEnvSafe("DB_PORT");
    std::string dbname = getEnvSafe("DB_NAME");
    std::string user = getEnvSafe("DB_USER");
    std::string password = getEnvSafe("DB_PASSWORD");
    std::string timeout = getEnvSafe("DB_CONNECT_TIMEOUT");

    return "host=" + (host.empty() ? "localhost" : host) +
           " port=" + (port.empty() ? "5432" : port) +
           " dbname=" + (dbname.empty() ? "crowapi_db" : dbname) +
           " user=" + (user.empty() ? "postgres" : user) +
           " password=" + (password.empty() ? "postgres" : password) +
           " connect_timeout=" + (timeout.empty() ? "5" : timeout);
}

PostgresDb::PostgresDb(std::string connStr, size_t maxPoolSize)
    : m_connStr(connStr.empty() ? getDefaultConnectionString() : std::move(connStr)),
      m_maxPoolSize(maxPoolSize) {
    testConnection();
}

PostgresDb::~PostgresDb() {
    std::lock_guard lock(m_mutex);
    m_pool.clear();
}

std::shared_ptr<pqxx::connection> PostgresDb::getConnection() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]() {
        return !m_pool.empty() || m_createdCount < m_maxPoolSize;
    });

    std::unique_ptr<pqxx::connection> conn;
    if (!m_pool.empty()) {
        conn = std::move(m_pool.back());
        m_pool.pop_back();
    } else {
        try {
            conn = std::make_unique<pqxx::connection>(m_connStr);
            ++m_createdCount;
        } catch (const std::exception& ex) {
            std::cerr << "[PostgresDb] Connection creation failed: " << ex.what() << std::endl;
            throw;
        }
    }

    if (!conn->is_open()) {
        try {
            conn = std::make_unique<pqxx::connection>(m_connStr);
        } catch (const std::exception& ex) {
            std::cerr << "[PostgresDb] Failed to reconnect: " << ex.what() << std::endl;
            if (m_createdCount > 0) {
                --m_createdCount;
            }
            m_cv.notify_one();
            throw;
        }
    }

    pqxx::connection* rawPtr = conn.release();
    std::weak_ptr<PostgresDb> weakSelf;
    try {
        weakSelf = weak_from_this();
    } catch (const std::bad_weak_ptr&) {
        // In case instance is not managed by shared_ptr
    }

    return std::shared_ptr<pqxx::connection>(rawPtr, [weakSelf](pqxx::connection* p) {
        if (auto self = weakSelf.lock()) {
            self->returnConnection(p);
        } else {
            delete p;
        }
    });
}

void PostgresDb::returnConnection(pqxx::connection* conn) {
    if (!conn) return;
    {
        std::lock_guard lock(m_mutex);
        if (conn->is_open()) {
            m_pool.push_back(std::unique_ptr<pqxx::connection>(conn));
        } else {
            delete conn;
            if (m_createdCount > 0) {
                --m_createdCount;
            }
        }
    }
    m_cv.notify_one();
}

bool PostgresDb::testConnection() {
    try {
        pqxx::connection conn(m_connStr);
        return conn.is_open();
    } catch (const std::exception& ex) {
        std::cerr << "[PostgresDb] Connection test failed: " << ex.what() << std::endl;
        return false;
    }
}

} // namespace Infrastructure::Persistence
