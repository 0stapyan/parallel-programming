cmake_minimum_required(VERSION 3.16)
project(thread_pool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(thread_pool main.cpp thread_pool.cpp)
target_link_libraries(thread_pool PRIVATE Threads::Threads)
