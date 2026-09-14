#ifndef CONNECTIONSPOOL_H
#define CONNECTIONSPOOL_H

#include <string>
#include <pqxx/pqxx>
#include <queue>
#include <memory>
#include <condition_variable>
#include <mutex>

namespace async_psql {

class ConnectionsPool {
public:
    ConnectionsPool(std::size_t kConnectionsCount, std::string connectionStr);

    std::unique_ptr<pqxx::connection> getConnection();
    void putConnection(std::unique_ptr<pqxx::connection> connection);

private:
    std::queue<std::unique_ptr<pqxx::connection>> m_connectionsQueue;
    std::condition_variable m_cv;
    std::mutex m_mutex;
};

} // namespace async_psql

#endif // CONNECTIONSPOOL_H
