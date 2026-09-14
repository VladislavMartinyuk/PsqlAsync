cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(PsqlAsync LANGUAGES CXX)

find_package(libpqxx CONFIG REQUIRED)

add_executable(PsqlAsync src/main.cpp
    src/threadpool.h src/threadpool.cpp
    src/asyncsqlawaitable.h
    src/connectionspool.h src/connectionspool.cpp
    src/enviroment.h src/enviroment.cpp
    src/shared.h
    async_psql/async_psql.h
    async_psql/async_psql.cpp)

target_link_libraries(PsqlAsync PRIVATE libpqxx::pqxx)
target_include_directories(PsqlAsync PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/async_psql)

include(GNUInstallDirs)
install(TARGETS PsqlAsync
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
