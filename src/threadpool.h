#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <vector>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <memory>

namespace async_psql {

using ThreadTask = std::function<void()>;

class ThreadPool {
public:
    ThreadPool(int kThreadsCount);
    ~ThreadPool();

    template <typename T, typename Func, typename... Args>
    std::future<T> post(Func &&f, Args &&...args) {
        auto funcTask = std::make_shared<std::packaged_task<T()>>(
            [f = std::forward<Func>(f), ... args = std::forward<Args>(args)]() mutable {
                if constexpr (std::is_invocable_r_v<T, Func, Args...>) {
                    return std::invoke(f, std::forward<Args>(args)...);
                }
                return T{};
            });
        auto fut = funcTask->get_future();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stopFlag.load()) {
                return std::future<T>{};
            }
            auto func = [funcTask]() { (*funcTask)(); };
            m_tasks.push(std::move(func));
        }
        m_cv.notify_one();
        return fut;
    }

private:
    void worker_loop();

    std::atomic<bool> m_stopFlag{false};
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<ThreadTask> m_tasks;
    std::vector<std::thread> m_workers;
};

} // namespace async_psql

#endif // THREADPOOL_H