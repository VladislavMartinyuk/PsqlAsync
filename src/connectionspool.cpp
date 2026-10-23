#include "connectionspool.h"

#include <vector>
#include <iostream>
#include <future>

using namespace async_psql;

ConnectionsPool::Lease::Lease(std::shared_ptr<ConnectionsPool> pool,
                              std::unique_ptr<pqxx::connection> connection)
    : m_pool(std::move(pool))
    , m_connection(std::move(connection)) {}

ConnectionsPool::Lease &ConnectionsPool::Lease::operator=(Lease &&other) noexcept {
    if (this != &other) {
        release();
        m_pool = std::move(other.m_pool);
        m_connection = std::move(other.m_connection);
    }
    return *this;
}

ConnectionsPool::Lease::~Lease() {
    release();
}

void ConnectionsPool::Lease::release() {
    if (m_pool) {
        m_pool->putConnection(std::move(m_connection));
        m_pool.reset();
    }
}

ConnectionsPool::ConnectionsPool(std::size_t kConnectionsCount, std::string connectionStr)
    : m_connectionStr(std::move(connectionStr)) {
    std::vector<std::future<std::unique_ptr<pqxx::connection>>> newConnections;
    newConnections.reserve(kConnectionsCount);
    for (std::size_t i = 0; i < kConnectionsCount; i++) {
        newConnections.emplace_back(std::async(std::launch::async, [this]() {
            try {
                return std::make_unique<pqxx::connection>(m_connectionStr);
            } catch (std::exception &e) {
                std::cerr << "[async_psql] connect error, will retry on first use: " << e.what()
                          << std::endl;
                return std::unique_ptr<pqxx::connection>{nullptr};
            }
        }));
    }
    for (auto &f : newConnections) {
        // Failed connections still occupy a slot, so the pool size stays as configured.
        putConnection(f.get());
    }
}

ConnectionsPool::Lease ConnectionsPool::acquire() {
    std::unique_ptr<pqxx::connection> connection;
    {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, [this]() { return !m_connectionsQueue.empty(); });
        connection = std::move(m_connectionsQueue.front());
        m_connectionsQueue.pop();
    }
    if (!connection || !connection->is_open()) {
        try {
            connection = std::make_unique<pqxx::connection>(m_connectionStr);
        } catch (...) {
            putConnection(nullptr);
            throw;
        }
    }
    return Lease{shared_from_this(), std::move(connection)};
}

void ConnectionsPool::putConnection(std::unique_ptr<pqxx::connection> connection) {
    {
        std::lock_guard lock(m_mutex);
        m_connectionsQueue.push(std::move(connection));
    }
    m_cv.notify_one();
}
