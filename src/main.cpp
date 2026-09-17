#include <iostream>

#include <boost/asio.hpp>

#include "async_psql.h"

namespace asio = boost::asio;

asio::awaitable<std::optional<pqxx::result>> coroSqlTask(const std::string &sql,
                                                         const std::string &dbName = "") {
    std::optional<pqxx::result> result = co_await async_psql::asio_exec(sql, dbName);
    std::cout << "Get result!" << std::endl;
    co_return result;
}

int main() {
    asio::io_context io_ctx;

    int threadsCount = std::thread::hardware_concurrency();
    async_psql::set_thread_pool_size(threadsCount);
    async_psql::add_db("127.0.0.1", "5432", "spl_db", "spl_user", "spl_uav_system");

    asio::co_spawn(
        io_ctx,
        []() -> asio::awaitable<void> {
            co_await coroSqlTask("SELECT pid, usename, datname, state, query, "
                                 "age(clock_timestamp(), query_start) AS duration "
                                 "FROM pg_stat_activity "
                                 "WHERE state != 'idle';",
                                 "spl_db");
        },
        asio::detached);

    io_ctx.run();

    return 0;
}