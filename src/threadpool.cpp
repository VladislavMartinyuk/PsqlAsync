#include "threadpool.h"
#include <algorithm>

using namespace async_psql;

ThreadPool::ThreadPool(int kThreadsCount) {
    int kThreads = std::max(kThreadsCount, 1);
    m_workers.reserve(kThreads);
    for (std::size_t i = 0; i < kThreads; ++i) {
        m_workers.emplace_back([this]() { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopFlag.store(true);
    }
    m_cv.notify_all();

    for (auto &thread : m_workers) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}


void ThreadPool::worker_loop() {
    while (true) {
        ThreadTask task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() { return m_stopFlag.load() || !m_tasks.empty(); });
            if (m_stopFlag.load() && m_tasks.empty()) {
                return;
            }
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        if (task) {
            task();
        }
    }
}