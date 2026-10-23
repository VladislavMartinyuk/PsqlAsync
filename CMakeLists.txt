cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(PsqlAsync LANGUAGES CXX)

find_package(libpqxx CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED)

add_library(PsqlAsyncLib    src/threadpool.h src/threadpool.cpp
    src/connectionspool.h src/connectionspool.cpp
    src/config.h src/config.cpp
    src/params.h
    src/asyncrun.h
    src/transaction.h src/transaction.cpp
    src/database.h src/database.cpp
    src/shared.h
    async_psql/async_psql.h )

#add_executable(PsqlAsync src/main.cpp)

target_link_libraries(PsqlAsyncLib PUBLIC libpqxx::pqxx Boost::boost)
target_include_directories(PsqlAsyncLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/async_psql)
#target_link_libraries(PsqlAsync PRIVATE PsqlAsyncLib)

#include(GNUInstallDirs)
#install(TARGETS PsqlAsync
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#)
