#ifndef ASYNC_PSQL_H
#define ASYNC_PSQL_H

#include "../src/enviroment.h"

namespace async_psql {

async_psql::Enviroment &async_env();
std::shared_ptr<async_psql::ThreadPool> get_thread_pool();
void set_thread_pool_size(int threadsCount = 1);
void add_db(std::string host,
            std::string port,
            std::string dbName,
            std::string user,
            std::string pass,
            int connectionsCount = 1);

} // namespace async_psql

#endif // ASYNC_PSQL_H
