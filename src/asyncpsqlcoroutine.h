#ifndef ASYNCPSQLCOROUTINE_H
#define ASYNCPSQLCOROUTINE_H

#include <coroutine>
#include <optional>
#include <exception>
#include <mutex>
#include <condition_variable>

namespace async_psql {

template <typename T> class AsyncPsqlCoroutine {
public:
    struct promise_type {
        std::optional<T> value{std::nullopt};
        std::exception_ptr exception{nullptr};
        std::condition_variable cv;
        std::mutex mutex;

        AsyncPsqlCoroutine get_return_object() {
            return AsyncPsqlCoroutine{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void return_value(T v) {
            {
                std::lock_guard lock(mutex);
                value = std::move(v);
            }
            cv.notify_one();
        }

        void unhandled_exception() {
            std::lock_guard lock(mutex);
            exception = std::current_exception();
            cv.notify_one();
        }
    };

    AsyncPsqlCoroutine(const AsyncPsqlCoroutine &) = delete;
    AsyncPsqlCoroutine &operator=(const AsyncPsqlCoroutine &) = delete;

    AsyncPsqlCoroutine(AsyncPsqlCoroutine &&other) noexcept
        : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }

    AsyncPsqlCoroutine &operator=(AsyncPsqlCoroutine &&other) noexcept {
        if (this != &other) {
            if (m_handle) {
                m_handle.destroy();
            }
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    explicit AsyncPsqlCoroutine(std::coroutine_handle<promise_type> h)
        : m_handle(h) {}

    ~AsyncPsqlCoroutine() {
        if (m_handle) {
            m_handle.destroy();
        }
    }

    std::optional<T> getResult() {
        if (!m_handle) {
            return std::nullopt;
        }

        std::unique_lock lock(m_handle.promise().mutex);
        m_handle.promise().cv.wait(lock, [this]() {
            return m_handle.promise().value.has_value() || m_handle.promise().exception != nullptr;
        });

        if (m_handle.promise().exception) {
            std::rethrow_exception(m_handle.promise().exception);
        }

        return std::move(m_handle.promise().value);
    }

    std::optional<T> await_resume() { return getResult(); }
    bool done() const { return m_handle ? m_handle.done() : true; }

private:
    std::coroutine_handle<promise_type> m_handle{nullptr};
};

} // namespace async_psql

#endif // ASYNCPSQLCOROUTINE_H
