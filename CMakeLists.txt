cmake_minimum_required(VERSION 3.16)

project(PsqlAsync LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PsqlAsync src/main.cpp
    src/threadpool.h src/threadpool.cpp)

include(GNUInstallDirs)
install(TARGETS PsqlAsync
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
