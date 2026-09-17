#ifndef ENVIROMENT_H
#define ENVIROMENT_H

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <mutex>

#include "connectionspool.h"
#include "threadpool.h"
#include "shared.h"

namespace async_psql {

class Enviroment : public details::NoCopyble<Enviroment> {
public:
    static Enviroment &instance();

    Enviroment &registerDBPool(std::string host,
                               std::string port,
                               std::string dbName,
                               std::string user,
                               std::string pass,
                               int connectionsCount);
    Enviroment &setupThreadPool(int kThreadsCount = 1);
    std::optional<std::shared_ptr<ConnectionsPool>> getDBPool(const std::string &dbName) const;
    std::shared_ptr<ThreadPool> getThreadPool() const;

private:
    Enviroment() = default;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<ConnectionsPool>> m_pools;
    std::shared_ptr<ThreadPool> m_threadPool{nullptr};
    std::once_flag m_threadPoolOnceFlag;
};

} // namespace async_psql

#endif // ENVIROMENT_H
