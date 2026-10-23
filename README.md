# async_psql 🚀

**async_psql** — это асинхронная библиотека-обёртка на C++20 для работы с базой данных **PostgreSQL** (через `libpqxx`) с поддержкой **C++20 Coroutines** и интеграцией с **Boost.Asio**.

Главное назначение библиотеки — исключить блокировку цикла событий `Boost.Asio` или основного потока приложения во время выполнения запросов к БД. Блокирующие сетевые вызовы `libpqxx` автоматически выносятся в изолированный пул потоков.

---

## ✨ Ключевые возможности

* **C++20 Coroutines**: `co_await db.exec(...)` внутри `boost::asio::awaitable`.
* **Любой completion token Asio**: `use_awaitable`, `as_tuple`, `use_future`, `deferred` или обычный колбэк.
* **Ошибки как исключения**: `pqxx::sql_error` и его наследники долетают до `co_await`.
* **Параметризованные запросы**: значения передаются отдельно от текста SQL, без SQL-инъекций.
* **Транзакции**: несколько запросов на одном соединении, автоматический откат, если не было `commit()`.
* **Пул подключений**: RAII-аренда соединений и автоматическое переподключение после обрыва.
* **Несколько БД**: каждый объект `Database` — отдельная база со своим пулом.

## 📦 Зависимости

* Компилятор с поддержкой **C++20** (GCC 10+, Clang 11+, MSVC 2019+)
* **CMake** 3.16+
* **Boost** 1.80+ (компонент `asio`)
* **libpqxx** 7.9+ / 8.x
* **libpq** (системная библиотека C для PostgreSQL)

---

## 🚀 Быстрый старт

```cpp
#include <boost/asio.hpp>
#include "async_psql.h"

namespace asio = boost::asio;

asio::awaitable<void> run(async_psql::Database db) {
    auto rows = co_await db.exec("SELECT id, name FROM users WHERE age > $1", 18);
    for (auto row : rows) {
        std::cout << row["id"].as<int>() << " " << row["name"].c_str() << "\n";
    }
}

int main() {
    asio::io_context io_ctx;

    async_psql::Database db{io_ctx.get_executor(),
                            {.host = "127.0.0.1",
                             .port = 5432,
                             .dbname = "spl_db",
                             .user = "spl_user",
                             .password = "secret",
                             .pool_size = 4}};

    asio::co_spawn(io_ctx, run(db), asio::detached);
    io_ctx.run();
}
```

`Database` — лёгкий копируемый дескриптор: копии используют один и тот же пул соединений и пул потоков. Передавайте его в корутины по значению. Если нужна вторая база данных, создайте второй объект `Database`.

### Config

| Поле        | По умолчанию  | Смысл                                                    |
|-------------|---------------|----------------------------------------------------------|
| `host`      | `"127.0.0.1"` |                                                          |
| `port`      | `5432`        |                                                          |
| `dbname`    |               |                                                          |
| `user`      |               |                                                          |
| `password`  |               | Любые символы, значения экранируются                     |
| `pool_size` | `1`           | Количество соединений                                    |
| `threads`   | `0`           | Рабочие потоки; `0` — по одному на соединение            |

## 🛡 Параметризованные запросы

Никогда не подставляйте пользовательские данные в текст запроса конкатенацией. Используйте плейсхолдеры `$1, $2, ...` и передавайте значения отдельными аргументами — они уходят в PostgreSQL отдельно от текста запроса и не интерпретируются как SQL:

```cpp
std::string name = request.get("name"); // недоверенные данные
co_await db.exec("SELECT * FROM users WHERE name = $1 AND age > $2", name, 18);
```

Значения копируются при вызове, поэтому можно передавать `std::string_view`, `const char *` и временные объекты. `nullptr`, пустой `std::optional` и нулевой `const char *` передаются как `NULL`.

Имена таблиц и колонок параметризовать нельзя — не берите их из пользовательского ввода (используйте белый список допустимых имён).

## ⚠️ Обработка ошибок

Ошибки SQL и соединения приходят как исключения `libpqxx` прямо в точку `co_await`:

```cpp
try {
    co_await db.exec("INSERT INTO users(email) VALUES ($1)", email);
} catch (const pqxx::unique_violation &) {
    // такой email уже есть
} catch (const pqxx::sql_error &e) {
    std::cerr << e.what() << "\nQuery: " << e.query() << "\n";
}
```

Если исключения не нужны, используйте `as_tuple`:

```cpp
auto [error, rows] = co_await db.async_exec("SELECT 1", {}, asio::as_tuple(asio::use_awaitable));
if (error) { /* std::exception_ptr */ }
```

## 🔁 Транзакции

```cpp
auto tx = co_await db.begin();
co_await tx.exec("UPDATE accounts SET balance = balance - $1 WHERE id = $2", sum, from);
co_await tx.exec("UPDATE accounts SET balance = balance + $1 WHERE id = $2", sum, to);
co_await tx.commit();
```

* `Transaction` держит одно соединение из пула всё время своей жизни.
* Если не вызвать `commit()` (например, вылетело исключение), деструктор откатит транзакцию.
* После `commit()` / `rollback()` соединение сразу возвращается в пул, а дальнейшие запросы через этот объект бросают `std::logic_error`.

> Не вызывайте `db.exec(...)`, пока держите открытую транзакцию, если в пуле одно соединение: запрос будет ждать соединение, которое занято вашей же транзакцией.

## 🎛 Completion tokens

У каждой операции есть две формы:

| Короткая форма (`use_awaitable`)   | Общая форма (любой токен)                      |
|------------------------------------|------------------------------------------------|
| `db.exec(sql, args...)`            | `db.async_exec(sql, make_params(args...), token)` |
| `db.begin()`                       | `db.async_begin(token)`                        |
| `tx.exec(sql, args...)`            | `tx.async_exec(sql, make_params(args...), token)` |
| `tx.commit()` / `tx.rollback()`    | `tx.async_commit(token)` / `tx.async_rollback(token)` |

Синхронный вызов без `io_context` (например, в CLI-утилите):

```cpp
asio::thread_pool callbacks{1};
async_psql::Database db{callbacks.get_executor(), config};

pqxx::result rows = db.async_exec("SELECT now()", {}, asio::use_future).get();
```

Колбэк:

```cpp
db.async_exec("SELECT 1", {}, [](std::exception_ptr error, pqxx::result rows) { ... });
```

Обработчик вызывается на своём связанном executor'е, а если его нет — на executor'е, переданном в `Database`.
