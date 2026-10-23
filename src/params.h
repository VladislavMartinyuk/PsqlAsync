#ifndef PARAMS_H
#define PARAMS_H

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <pqxx/pqxx>

namespace async_psql {

namespace details {

// pqxx::params keeps zview/string_view and binary lvalues as non-owning views,
// but the query runs later on a pool thread, so every value must be copied.
template <typename T> decltype(auto) owned_param(T &&value) {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_pointer_v<U> &&
                  std::is_same_v<std::remove_cv_t<std::remove_pointer_t<U>>, char>) {
        // A null C string becomes SQL NULL instead of undefined behaviour.
        return value ? std::optional<std::string>{value} : std::nullopt;
    } else if constexpr (std::is_convertible_v<const U &, std::string_view> &&
                         !std::is_same_v<U, std::string> &&
                         !std::is_same_v<U, std::nullptr_t>) {
        return std::string{std::string_view{value}};
    } else if constexpr (pqxx::binary<U>) {
        return pqxx::bytes(std::begin(value), std::end(value));
    } else {
        return std::forward<T>(value);
    }
}

inline pqxx::result exec_query(pqxx::transaction_base &trx,
                               const std::string &sql,
                               const pqxx::params &params) {
    // Without parameters keep plain exec(): it allows multi-statement scripts,
    // which PQexecParams does not.
    if (params.size() == 0) {
        return trx.exec(sql);
    }
    return trx.exec(sql, params);
}

} // namespace details

// Values for $1, $2, ... placeholders. They are sent separately from the query
// text, so they are never interpreted as SQL.
template <typename... Args> pqxx::params make_params(Args &&...args) {
    pqxx::params params;
    params.reserve(sizeof...(args));
    (params.append(details::owned_param(std::forward<Args>(args))), ...);
    return params;
}

} // namespace async_psql

#endif // PARAMS_H
