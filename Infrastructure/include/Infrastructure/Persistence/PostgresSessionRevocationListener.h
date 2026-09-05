#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

namespace Infrastructure::Persistence {

class PostgresSessionRevocationListener {
public:
    using RevocationCallback = std::function<void(std::string_view sessionId)>;

    explicit PostgresSessionRevocationListener(
        std::string connectionString,
        RevocationCallback onRevoke = nullptr
    );
    ~PostgresSessionRevocationListener();

    void start();
    void stop();
    bool isRunning() const noexcept { return m_running; }

private:
    void runListener();

    std::string m_connectionString;
    RevocationCallback m_callback;
    std::atomic<bool> m_running{false};
    std::thread m_workerThread;
};

} // namespace Infrastructure::Persistence
