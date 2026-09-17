#ifndef ASYNCSQLAWAITABLE_H
#define ASYNCSQLAWAITABLE_H

#include <coroutine>
#include <string>
#include <pqxx/pqxx>
#include <optional>
#include <iostream>
#include <boost/asio.hpp>

#include "enviroment.h"

namespace async_psql {

class AsyncSqlAwaitable {
public:
    AsyncSqlAwaitable(std::string sql)
        : m_sqlQuery(std::move(sql)) {}
    AsyncSqlAwaitable(std::string sql, std::string dbName)
        : m_sqlQuery(std::move(sql))
        , m_dbName(std::move(dbName)) {}

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        auto pool = Enviroment::instance().getThreadPool();
        pool->post<void>([h, this]() {
            auto dbPool = Enviroment::instance().getDBPool(m_dbName);
            if (dbPool.has_value()) {
                auto connection = dbPool.value()->getConnection();
                try {
                    pqxx::work trx{*connection};
                    auto result = trx.exec(m_sqlQuery);
                    m_result = std::move(result);
                    trx.commit();
                } catch (const std::exception &e) {
                    std::cerr << "Sql error: " << e.what() << std::endl;
                }
                dbPool.value()->putConnection(std::move(connection));
            }
            h.resume();
        });
    }

    template <typename CompletionToken> auto async_dispatch(CompletionToken &&token) {
        return boost::asio::async_initiate<CompletionToken, void(std::optional<pqxx::result>)>(
            [sql = m_sqlQuery, dbName = m_dbName](auto handler) mutable {
                auto pool = Enviroment::instance().getThreadPool();
                pool->post<void>([sql = std::move(sql),
                                  dbName = std::move(dbName),
                                  handler = std::move(handler)]() mutable {
                    std::optional<pqxx::result> res = std::nullopt;
                    auto dbPool = Enviroment::instance().getDBPool(dbName);

                    if (dbPool.has_value()) {
                        auto connection = dbPool.value()->getConnection();
                        try {
                            pqxx::work trx{*connection};
                            res = trx.exec(sql);
                            trx.commit();
                        } catch (const std::exception &e) {
                            std::cerr << "[async_psql] SQL Exception: " << e.what() << std::endl;
                        }
                        dbPool.value()->putConnection(std::move(connection));
                    } else {
                        std::cerr << "[async_psql] Error: DB Pool not found for name: '" << dbName
                                  << "'\n";
                    }

                    // Передаем результат дальше в Asio handler
                    std::move(handler)(res);
                });
            },
            token);
    }

    std::optional<pqxx::result> await_resume() noexcept { return m_result; }

private:
    std::string m_sqlQuery;
    std::string m_dbName;
    std::optional<pqxx::result> m_result{std::nullopt};
};

} // namespace async_psql

#endif // ASYNCSQLAWAITABLE_H
