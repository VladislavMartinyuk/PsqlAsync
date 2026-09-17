# async_psql 🚀

**async_psql** — это асинхронная библиотека-обёртка на C++20 для работы с базой данных **PostgreSQL** (через `libpqxx`) с поддержкой **C++20 Coroutines** и интеграцией с **Boost.Asio**.

Главное назначение библиотеки — исключить блокировку цикла событий `Boost.Asio` или основного потока приложения во время выполнения запросов к БД. Блокирующие сетевые вызовы `libpqxx` автоматически выносятся в изолированный пул потоков.

---

## ✨ Ключевые возможности

* **C++20 Coroutines**: Из коробки поддерживается удобный синтаксис `co_await`.
* **Интеграция с Boost.Asio**: Готовый CompletionToken `async_psql::asio_exec` для использования внутри `boost::asio::awaitable`.
* **Пул подключений (Connections Pool)**: Автоматическое распределение, переиспользование и потокобезопасный выбор подключений.
* **Поддержка нескольких БД**: Возможность параллельной работы с несколькими базами данных по их текстовым именам/алиасам.
* **Собственный ThreadPool**: Выполнение транзакций `pqxx::work` в изолированном пуле рабочих потоков.

## 📦 Зависимости

Для сборки и работы библиотеки требуется:
* Компилятор с поддержкой **C++20** (GCC 10+, Clang 11+, MSVC 2019+)
* **Boost** (компонент `asio`)
* **libpqxx**
* **libpq** (системная библиотека C для PostgreSQL)

---

## 🚀 Быстрый старт

### 🛠 Инициализация библиотеки

Перед выполнением запросов необходимо настроить размер пула потоков и добавить параметры подключения к базе данных:

```cpp
#include "async_psql.h"

int main() {
    // 1. Устанавливаем количество потоков для исполнения SQL-запросов
    async_psql::set_thread_pool_size(std::thread::hardware_concurrency());

    // 2. Регистрируем БД: Host, Port, DBName, User, Password, connectionsCount
    async_psql::add_db("127.0.0.1", "5432", "spl_db", "spl_user", "spl_uav_system", 5);

    return 0;
}

```

## Примеры использования

# Использование с Boost.Asio (asio_exec)

Этот способ предпочтителен для сетевых служб, асинхронных серверов и веб-приложений на базе Boost.Asio.

```cpp
#include <iostream>
#include <boost/asio.hpp>
#include "async_psql.h"

namespace asio = boost::asio;

// Асинхронная задача Boost.Asio
asio::awaitable<std::optional<pqxx::result>> coroSqlTask(const std::string &sql,
                                                         const std::string &dbName = "") {
    // Асинхронно выполняем запрос к БД без блокировки io_context
    std::optional<pqxx::result> result = co_await async_psql::asio_exec(sql, dbName);
    std::cout << "Запрос успешно выполнен!" << std::endl;
    co_return result;
}

int main() {
    asio::io_context io_ctx;

    // Настройка окружения
    int threadsCount = std::thread::hardware_concurrency();
    async_psql::set_thread_pool_size(threadsCount);
    async_psql::add_db("127.0.0.1", "5432", "spl_db", "spl_user", "spl_uav_system");

    // Спавн корутины
    asio::co_spawn(
        io_ctx,
        []() -> asio::awaitable<void> {
            auto res = co_await coroSqlTask(
                "SELECT pid, usename, datname, state, query, "
                "age(clock_timestamp(), query_start) AS duration "
                "FROM pg_stat_activity "
                "WHERE state != 'idle';",
                "spl_db");

            if (res.has_value()) {
                std::cout << "Получено строк из БД: " << res->size() << std::endl;
            }
        },
        asio::detached);

    // Запуск цикла событий
    io_ctx.run();

    return 0;
}
```

# Использование вне Boost.Asio (co_exec и AsyncPsqlCoroutine)

Если вам нужно использовать корутины без контекста Boost.Asio (например, в синхронном CLI-приложении), библиотека предоставляет обёртку AsyncPsqlCoroutine.

```cpp

#include <iostream>
#include "async_psql.h"
#include "asyncpsqlcoroutine.h"

// Кастомная C++20 корутина
async_psql::AsyncPsqlCoroutine<pqxx::result> getSystemStats() {
    std::string sql = "SELECT pid, usename, state FROM pg_stat_activity WHERE state != 'idle';";
    
    // Ожидание выполнения запроса
    std::optional<pqxx::result> res = co_await async_psql::co_exec(sql, "spl_db");
    
    if (res.has_value()) {
        co_return *res;
    }
    co_return pqxx::result{};
}

int main() {
    async_psql::set_thread_pool_size(2);
    async_psql::add_db("127.0.0.1", "5432", "spl_db", "spl_user", "spl_uav_system");

    // Запуск корутины
    auto task = getSystemStats();
    
    // Блокирующий забор результата для главного потока
    std::optional<pqxx::result> result = task.getResult();

    if (result.has_value()) {
        std::cout << "Строк получено: " << result->size() << std::endl;
    }

    return 0;
}

```

## Зависимости
    1.C++20 compiler (GCC 10+, Clang 11+, MSVC 2019+)
    2.CMake 3.16+
    3.libpqxx (C++ PostgreSQL library)
    4.libpq (PostgreSQL client library)
    5.Boost 1.70+ with Asio