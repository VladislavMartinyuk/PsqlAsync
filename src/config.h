#ifndef CONFIG_H
#define CONFIG_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace async_psql {

struct Config {
    std::string host{"127.0.0.1"};
    std::uint16_t port{5432};
    std::string dbname;
    std::string user;
    std::string password;
    std::size_t pool_size{1};
    // 0 means "one worker thread per connection".
    std::size_t threads{0};

    std::string connectionString() const;
};

} // namespace async_psql

#endif // CONFIG_H
