#include "config.h"

#include <format>
#include <string_view>

using namespace async_psql;

namespace {

// libpq conninfo value: wrap in single quotes, escape ' and \ with backslash,
// so spaces or "key=value" inside a value can't inject extra parameters.
std::string quoteConnParam(std::string_view value) {
    std::string result{"'"};
    for (char c : value) {
        if (c == '\'' || c == '\\') {
            result += '\\';
        }
        result += c;
    }
    result += '\'';
    return result;
}

} // namespace

std::string Config::connectionString() const {
    return std::format("host={} port={} dbname={} user={} password={}",
                       quoteConnParam(host),
                       port,
                       quoteConnParam(dbname),
                       quoteConnParam(user),
                       quoteConnParam(password));
}
