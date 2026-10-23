#include "database.h"

#include <algorithm>

using namespace async_psql;

Database::Database(boost::asio::any_io_executor executor, Config config) {
    auto poolSize = std::max<std::size_t>(config.pool_size, 1);
    // Every query blocks a worker for its whole duration, so fewer threads than
    // connections would leave connections idle.
    auto threads = config.threads != 0 ? config.threads : poolSize;

    m_core = std::make_shared<details::DatabaseCore>();
    m_core->executor = std::move(executor);
    m_core->connections = std::make_shared<ConnectionsPool>(poolSize, config.connectionString());
    m_core->threads = std::make_unique<ThreadPool>(static_cast<int>(threads));
}
