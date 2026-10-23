#include "transaction.h"

using namespace async_psql;
using namespace async_psql::details;

std::shared_ptr<TransactionState> TransactionState::begin(ConnectionsPool &pool) {
    auto state = std::make_shared<TransactionState>();
    state->lease = pool.acquire();
    state->work.emplace(*state->lease);
    return state;
}

pqxx::result TransactionState::exec(const std::string &sql, const pqxx::params &params) {
    std::lock_guard lock(mutex);
    ensureActive();
    return exec_query(*work, sql, params);
}

void TransactionState::commit() {
    std::lock_guard lock(mutex);
    ensureActive();
    work->commit();
    // Give the connection back right away, not when the Transaction object dies.
    work.reset();
    lease.release();
}

void TransactionState::rollback() {
    std::lock_guard lock(mutex);
    ensureActive();
    work->abort();
    work.reset();
    lease.release();
}

void TransactionState::ensureActive() const {
    if (!work) {
        throw std::logic_error("async_psql: transaction is already committed or rolled back");
    }
}

Transaction &Transaction::operator=(Transaction &&other) noexcept {
    if (this != &other) {
        dispose();
        m_core = std::move(other.m_core);
        m_state = std::move(other.m_state);
    }
    return *this;
}

Transaction::~Transaction() {
    dispose();
}

void Transaction::dispose() noexcept {
    if (m_core && m_state) {
        // An unfinished pqxx::work sends ROLLBACK in its destructor. That is network I/O,
        // so do it on the pool instead of blocking the caller (usually the io_context thread).
        try {
            m_core->threads->post<void>([state = std::move(m_state)]() mutable { state.reset(); });
        } catch (...) {
        }
    }
    m_state.reset();
    m_core.reset();
}
