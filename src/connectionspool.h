#ifndef CONNECTIONSPOOL_H
#define CONNECTIONSPOOL_H

#include <string>
#include <pqxx/pqxx>
#include <queue>
#include <memory>
#include <condition_variable>
#include <mutex>

namespace async_psql {

class ConnectionsPool : public std::enable_shared_from_this<ConnectionsPool> {
public:
    // Owns a connection taken from the pool and gives it back on destruction.
    class Lease {
    public:
        Lease() = default;
        Lease(Lease &&other) noexcept = default;
        Lease &operator=(Lease &&other) noexcept;
        Lease(const Lease &) = delete;
        Lease &operator=(const Lease &) = delete;
        ~Lease();

        pqxx::connection &operator*() const { return *m_connection; }
        pqxx::connection *operator->() const { return m_connection.get(); }
        explicit operator bool() const { return m_connection != nullptr; }

        void release();

    private:
        friend class ConnectionsPool;
        Lease(std::shared_ptr<ConnectionsPool> pool, std::unique_ptr<pqxx::connection> connection);

        std::shared_ptr<ConnectionsPool> m_pool;
        std::unique_ptr<pqxx::connection> m_connection;
    };

    ConnectionsPool(std::size_t kConnectionsCount, std::string connectionStr);

    // Blocks until a slot is free; (re)connects if the slot has no live connection.
    // Must be called on an object owned by std::shared_ptr.
    Lease acquire();

private:
    void putConnection(std::unique_ptr<pqxx::connection> connection);

    std::string m_connectionStr;
    // nullptr is a free slot without a connection: it is connected lazily in acquire().
    std::queue<std::unique_ptr<pqxx::connection>> m_connectionsQueue;
    std::condition_variable m_cv;
    std::mutex m_mutex;
};

} // namespace async_psql

#endif // CONNECTIONSPOOL_H
