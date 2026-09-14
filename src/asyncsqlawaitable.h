#ifndef ASYNCSQLAWAITABLE_H
#define ASYNCSQLAWAITABLE_H

#include <coroutine>
#include <string>
#include <pqxx/pqxx>

namespace async_psql {

class AsyncSqlAwaitable {
public:
    AsyncSqlAwaitable(std::string sql)
        : m_sqlQuery(std::move(sql)) {}

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) {}
    pqxx::result await_resume() noexcept { return m_result; }

private:
    std::string m_sqlQuery;
    pqxx::result m_result;
};

} // namespace async_psql

#endif // ASYNCSQLAWAITABLE_H
