#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <pqxx/pqxx>
#include <boost/asio.hpp>

#include "asyncrun.h"
#include "connectionspool.h"
#include "params.h"

namespace async_psql {

namespace details {

// Everything that touches the network lives here and is used only on pool threads.
struct TransactionState {
    std::mutex mutex;
    // Declared before `work`: the transaction must end before the connection goes back.
    ConnectionsPool::Lease lease;
    std::optional<pqxx::work> work;

    static std::shared_ptr<TransactionState> begin(ConnectionsPool &pool);

    pqxx::result exec(const std::string &sql, const pqxx::params &params);
    void commit();
    void rollback();

private:
    void ensureActive() const;
};

} // namespace details

// Holds one connection from the pool for its whole lifetime.
// If neither commit() nor rollback() was awaited, the destructor rolls back.
// Operations of one transaction run one after another, never in parallel.
class Transaction {
public:
    Transaction() = default;
    Transaction(Transaction &&other) noexcept = default;
    Transaction &operator=(Transaction &&other) noexcept;
    Transaction(const Transaction &) = delete;
    Transaction &operator=(const Transaction &) = delete;
    ~Transaction();

    template <typename CompletionToken>
    auto async_exec(std::string sql, pqxx::params params, CompletionToken &&token) {
        return details::async_run<pqxx::result>(
            core(),
            [state = m_state, sql = std::move(sql), params = std::move(params)]() {
                return state->exec(sql, params);
            },
            std::forward<CompletionToken>(token));
    }

    template <typename... Args> auto exec(std::string sql, Args &&...args) {
        return async_exec(std::move(sql),
                          make_params(std::forward<Args>(args)...),
                          boost::asio::use_awaitable);
    }

    template <typename CompletionToken> auto async_commit(CompletionToken &&token) {
        return details::async_run<void>(
            core(), [state = m_state]() { state->commit(); }, std::forward<CompletionToken>(token));
    }

    auto commit() { return async_commit(boost::asio::use_awaitable); }

    template <typename CompletionToken> auto async_rollback(CompletionToken &&token) {
        return details::async_run<void>(
            core(), [state = m_state]() { state->rollback(); }, std::forward<CompletionToken>(token));
    }

    auto rollback() { return async_rollback(boost::asio::use_awaitable); }

private:
    friend class Database;
    Transaction(std::shared_ptr<details::DatabaseCore> core,
                std::shared_ptr<details::TransactionState> state)
        : m_core(std::move(core))
        , m_state(std::move(state)) {}

    const std::shared_ptr<details::DatabaseCore> &core() const {
        if (!m_core || !m_state) {
            throw std::logic_error("async_psql: operation on an empty Transaction");
        }
        return m_core;
    }

    void dispose() noexcept;

    std::shared_ptr<details::DatabaseCore> m_core;
    std::shared_ptr<details::TransactionState> m_state;
};

} // namespace async_psql

#endif // TRANSACTION_H
