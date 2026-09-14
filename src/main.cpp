#include <iostream>

#include "async_psql.h"

int main() {
    int threadsCount = std::thread::hardware_concurrency();
    async_psql::set_thread_pool_size(threadsCount);
    async_psql::add_db("127.0.0.1", "5432", "postgres", "postgres", "0000");
    async_psql::get_thread_pool()->post<void>([]() {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "Hello world!" << std::endl;
    });

    return 0;
}