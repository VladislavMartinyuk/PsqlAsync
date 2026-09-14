#include "enviroment.h"

#include <format>

using namespace async_psql;

Enviroment &Enviroment::instance() {
    static Enviroment inst;
    return inst;
}

Enviroment &Enviroment::registerDBPool(std::string host,
                                       std::string port,
                                       std::string dbName,
                                       std::string user,
                                       std::string pass,
                                       int connectionsCount) {
    std::string connectionStr = std::format("host={} port={} dbname={} user={} password={}",
                                            host,
                                            port,
                                            dbName,
                                            user,
                                            pass);
    auto newPool = std::make_shared<ConnectionsPool>(connectionsCount, std::move(connectionStr));
    {
        std::lock_guard lock(m_mutex);
        m_pools.insert({dbName, std::move(newPool)});
    }
    return *this;
}

Enviroment &Enviroment::setupThreadPool(int kThreadsCount) {
    std::call_once(m_threadPoolOnceFlag, [this, kThreadsCount]() {
        m_threadPool = std::make_shared<ThreadPool>(kThreadsCount);
    });
    return *this;
}

std::optional<std::shared_ptr<ConnectionsPool>>
Enviroment::getPool(const std::string &dbName) const {
    std::lock_guard lock(m_mutex);
    if (!m_pools.empty() && !dbName.empty() && m_pools.contains(dbName)) {
        return m_pools.at(dbName);
    } else if (!m_pools.empty() && dbName.empty()) {
        auto it = m_pools.begin();
        return it->second;
    } else {
        return std::nullopt;
    }
}

std::shared_ptr<ThreadPool> Enviroment::getThreadPool() const {
    return m_threadPool;
}
