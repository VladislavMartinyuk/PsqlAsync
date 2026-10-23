#include <iostream>

#include <boost/asio.hpp>

#include "async_psql.h"

namespace asio = boost::asio;

asio::awaitable<void> run(async_psql::Database db) {
    try {
        auto rows = co_await db.exec("SELECT pid, usename, datname, state "
                                     "FROM pg_stat_activity WHERE state != $1",
                                     "idle");
        std::cout << "Active sessions: " << rows.size() << std::endl;

        auto tx = co_await db.begin();
        co_await tx.exec("CREATE TEMP TABLE demo(id int, name text)");
        co_await tx.exec("INSERT INTO demo VALUES ($1, $2)", 1, "O'Reilly; DROP TABLE demo");
        auto inserted = co_await tx.exec("SELECT name FROM demo WHERE id = $1", 1);
        co_await tx.commit();
        std::cout << "Inserted: " << inserted[0][0].c_str() << std::endl;
    } catch (const pqxx::sql_error &e) {
        std::cerr << "SQL error: " << e.what() << "\nQuery: " << e.query() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    asio::io_context io_ctx;

    async_psql::Database db{io_ctx.get_executor(),
                            {.host = "127.0.0.1",
                             .port = 5432,
                             .dbname = "spl_db",
                             .user = "spl_user",
                             .password = "spl_uav_system",
                             .pool_size = 4}};

    asio::co_spawn(io_ctx, run(db), asio::detached);

    io_ctx.run();

    return 0;
}
