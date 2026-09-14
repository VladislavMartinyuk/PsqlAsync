#include "connectionspool.h"

#include <vector>
#include <iostream>
#include <future>

using namespace async_psql;

ConnectionsPool::ConnectionsPool(std::size_t kConnectionsCount, std::string connectionStr) {
    std::vector<std::future<std::unique_ptr<pqxx::connection>>> newConnections;
    newConnections.reserve(kConnectionsCount);
    for (int i = 0; i < kConnectionsCount; i++) {
        newConnections.emplace_back(std::async(std::launch::async, [&connectionStr]() {
            try {
                auto connection = std::make_unique<pqxx::connection>(connectionStr);
                return connection;
            } catch (std::exception &e) {
                std::cerr << "PSQL connect error: " << e.what() << std::endl;
                return std::unique_ptr<pqxx::connection>{nullptr};
            }
        }));
    }
    for (auto &f : newConnections) {
        auto conn = f.get();
        if (conn) {
            {
                std::lock_guard lock(m_mutex);
                m_connectionsQueue.push(std::move(conn));
            }
            m_cv.notify_one();
        }
    }
}

std::unique_ptr<pqxx::connection> ConnectionsPool::getConnection() {
    std::unique_ptr<pqxx::connection> result;
    {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, [this]() { return !m_connectionsQueue.empty(); });
        result = std::move(m_connectionsQueue.front());
        m_connectionsQueue.pop();
    }
    return result;
}

void ConnectionsPool::putConnection(std::unique_ptr<pqxx::connection> connection) {
    {
        std::lock_guard lock(m_mutex);
        m_connectionsQueue.push(std::move(connection));
    }
    m_cv.notify_one();
}
