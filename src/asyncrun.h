#ifndef ASYNCRUN_H
#define ASYNCRUN_H

#include <exception>
#include <memory>
#include <type_traits>
#include <boost/asio.hpp>

#include "connectionspool.h"
#include "threadpool.h"

namespace async_psql::details {

// State shared by a Database and all its Transactions.
// Work posted to the thread pool must never hold a pointer to it: if a worker
// dropped the last reference, ~ThreadPool would try to join its own thread.
struct DatabaseCore {
    boost::asio::any_io_executor executor;
    std::shared_ptr<ConnectionsPool> connections;
    std::unique_ptr<ThreadPool> threads;
};

template <typename Result> struct CompletionSignatureFor {
    using type = void(std::exception_ptr, Result);
};

template <> struct CompletionSignatureFor<void> {
    using type = void(std::exception_ptr);
};

template <typename Result>
using CompletionSignature = typename CompletionSignatureFor<Result>::type;

// Runs blocking libpqxx `work` on the thread pool and completes the handler on
// its associated executor (or the database executor) with (exception_ptr, result).
// use_awaitable turns a non-null exception_ptr into an exception at co_await.
template <typename Result, typename Work, typename CompletionToken>
auto async_run(const std::shared_ptr<DatabaseCore> &core, Work work, CompletionToken &&token) {
    return boost::asio::async_initiate<CompletionToken, CompletionSignature<Result>>(
        [](auto handler, std::shared_ptr<DatabaseCore> core, Work work) {
            auto executor = boost::asio::get_associated_executor(handler, core->executor);
            // Keeps io_context::run() from returning while the query is in flight.
            auto workGuard = boost::asio::make_work_guard(executor);
            auto operation = [work = std::move(work),
                              handler = std::move(handler),
                              workGuard = std::move(workGuard)]() mutable {
                std::exception_ptr error;
                if constexpr (std::is_void_v<Result>) {
                    try {
                        work();
                    } catch (...) {
                        error = std::current_exception();
                    }
                    boost::asio::post(workGuard.get_executor(),
                                      boost::asio::append(std::move(handler), error));
                } else {
                    Result result{};
                    try {
                        result = work();
                    } catch (...) {
                        error = std::current_exception();
                    }
                    boost::asio::post(
                        workGuard.get_executor(),
                        boost::asio::append(std::move(handler), error, std::move(result)));
                }
            };
            // Asio handlers are move-only, but ThreadPool tasks must be copyable
            // (std::function / MSVC std::packaged_task), so share the operation.
            auto shared = std::make_shared<decltype(operation)>(std::move(operation));
            core->threads->post<void>([shared]() { (*shared)(); });
        },
        token,
        core,
        std::move(work));
}

} // namespace async_psql::details

#endif // ASYNCRUN_H
