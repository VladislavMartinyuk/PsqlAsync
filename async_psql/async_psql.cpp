#include "async_psql.h"

namespace async_psql {

Enviroment &async_env() {
    return Enviroment::instance();
}

std::shared_ptr<ThreadPool> get_thread_pool() {
    return Enviroment::instance().getThreadPool();
}

void set_thread_pool_size(int threadsCount) {
    Enviroment::instance().setupThreadPool(threadsCount);
}

void add_db(std::string host,
            std::string port,
            std::string dbName,
            std::string user,
            std::string pass,
            int connectionsCount) {
    Enviroment::instance().registerDBPool(std::move(host),
                                          std::move(port),
                                          std::move(dbName),
                                          std::move(user),
                                          std::move(pass),
                                          std::move(connectionsCount));
}

} // namespace async_psql
