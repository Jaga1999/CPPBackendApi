#include "Infrastructure/Persistence/PostgresSessionRevocationListener.h"
#include <iostream>
#include <pqxx/pqxx>

namespace Infrastructure::Persistence {

PostgresSessionRevocationListener::PostgresSessionRevocationListener(
    std::string connectionString,
    RevocationCallback onRevoke
)   : m_connectionString(std::move(connectionString)),
      m_callback(std::move(onRevoke)) {}

PostgresSessionRevocationListener::~PostgresSessionRevocationListener() {
    stop();
}

void PostgresSessionRevocationListener::start() {
    if (m_running.exchange(true)) return;
    m_workerThread = std::thread(&PostgresSessionRevocationListener::runListener, this);
}

void PostgresSessionRevocationListener::stop() {
    if (!m_running.exchange(false)) return;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void PostgresSessionRevocationListener::runListener() {
    try {
        pqxx::connection conn{m_connectionString};
        conn.listen("session_revoked", [this](pqxx::notification notif) {
            if (m_callback) {
                m_callback(notif.payload);
            }
        });
        std::cout << "[PostgresSessionRevocationListener] Listening on channel 'session_revoked'...\n";

        while (m_running) {
            conn.await_notification(1, 0); // Check every 1 second
        }
        conn.listen("session_revoked", {});
    } catch (const std::exception& ex) {
        if (m_running) {
            std::cerr << "[PostgresSessionRevocationListener] Error: " << ex.what() << std::endl;
        }
    }
}

} // namespace Infrastructure::Persistence
