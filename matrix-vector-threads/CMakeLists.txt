cmake_minimum_required(VERSION 3.16)
project(matrix_vector_threads CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(benchmark main.cpp matrix_vector.cpp)
target_link_libraries(benchmark PRIVATE Threads::Threads)
