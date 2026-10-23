#ifndef DATABASE_H
#define DATABASE_H

#include <memory>
#include <string>
#include <pqxx/pqxx>
#include <boost/asio.hpp>

#include "asyncrun.h"
#include "config.h"
#include "params.h"
#include "transaction.h"

namespace async_psql {

// A cheap copyable handle: copies share one connection pool and one thread pool.
// Completion handlers run on their associated executor, or on `executor` by default.
class Database {
public:
    Database(boost::asio::any_io_executor executor, Config config);

    // One statement in its own transaction (autocommit).
    template <typename CompletionToken>
    auto async_exec(std::string sql, pqxx::params params, CompletionToken &&token) {
        return details::async_run<pqxx::result>(
            m_core,
            [connections = m_core->connections, sql = std::move(sql), params = std::move(params)]() {
                auto connection = connections->acquire();
                pqxx::work trx{*connection};
                auto result = details::exec_query(trx, sql, params);
                trx.commit();
                return result;
            },
            std::forward<CompletionToken>(token));
    }

    template <typename... Args> auto exec(std::string sql, Args &&...args) {
        return async_exec(std::move(sql),
                          make_params(std::forward<Args>(args)...),
                          boost::asio::use_awaitable);
    }

    template <typename CompletionToken> auto async_begin(CompletionToken &&token) {
        return boost::asio::async_initiate<CompletionToken,
                                           void(std::exception_ptr, Transaction)>(
            [](auto handler, std::shared_ptr<details::DatabaseCore> core) {
                auto executor = boost::asio::get_associated_executor(handler, core->executor);
                // The pool thread only produces the state; the Transaction (which owns
                // a reference to the core) is assembled back on the handler's executor.
                details::async_run<std::shared_ptr<details::TransactionState>>(
                    core,
                    [connections = core->connections]() {
                        return details::TransactionState::begin(*connections);
                    },
                    boost::asio::bind_executor(
                        executor,
                        [handler = std::move(handler), core](
                            std::exception_ptr error,
                            std::shared_ptr<details::TransactionState> state) mutable {
                            Transaction transaction;
                            if (state) {
                                transaction = Transaction{std::move(core), std::move(state)};
                            }
                            std::move(handler)(error, std::move(transaction));
                        }));
            },
            token,
            m_core);
    }

    auto begin() { return async_begin(boost::asio::use_awaitable); }

private:
    std::shared_ptr<details::DatabaseCore> m_core;
};

} // namespace async_psql

#endif // DATABASE_H
