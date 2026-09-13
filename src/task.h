#ifndef TASK_H
#define TASK_H

#include <coroutine>
#include <optional>
#include <exception>

namespace pcoro {

template <typename T> class Task {
public:
    struct promise_type {
        std::coroutine_handle<> caller;
        std::optional<T> value;

        auto get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        auto initial_suspend() { return std::suspend_never{}; }
        auto final_suspend() {
            struct FinalAwaiter {
                bool await_ready() { return false; }
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) {
                    if (h.promise().caller) {
                        return h.promise().caller;
                    }
                    return std::noop_coroutine();
                }
                void await_resume() {}
            };
            return FinalAwaiter{};
        }

        void return_value(T v) { value = std::move(v); }
        void unhandled_exception() { std::terminate(); }
    };

    bool await_ready() { return m_handle.promise().value.has_value(); }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) {
        m_handle.promise().caller = h;
        return m_handle;
    }
    T await_resume() { return m_handle.promise().value.value(); }

    Task(std::coroutine_handle<promise_type> h)
        : m_handle(h) {}

    ~Task() {
        if (m_handle) {
            m_handle.destroy();
        }
    }

    void resume() { m_handle.resume(); }

private:
    std::coroutine_handle<promise_type> m_handle;
};

/*
 *
 * Task<int> getInt() {
 *      std::cout << "getInt" << std::endl;
 *      co_return 52;
 * }
 *
 * Task<int> foo() {
 *      int a = co_await getInt();
 *      co_return a;
 * }
 *
 * int main() {
 *      auto coro = foo();
 *      return 0;
 * }
 *
 */

} // namespace pcoro

#endif // TASK_H
