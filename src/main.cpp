#include "threadpool.h"
#include <iostream>

struct Callable {
    std::string operator()() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "Func executing at thread: " << std::this_thread::get_id() << std::endl;
        return std::string{"Hello world"};
    }
};

int main() {
    std::cout << "Main thread id: " << std::this_thread::get_id() << std::endl;

    auto res = async_psql::ThreadPool::instance().post<std::string>(Callable{});

    std::cout << "Result: " << res.get() << std::endl;
    return 0;
}