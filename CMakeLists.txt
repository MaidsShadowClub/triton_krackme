cmake_minimum_required(VERSION 3.5)

project(triton_krackme LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LIEF REQUIRED COMPONENTS STATIC)
find_package(triton REQUIRED CONFIG)

add_executable(triton_krackme main.cpp utils.hpp hooks.hpp)
add_library(utils STATIC utils.cpp)
add_library(hooks STATIC hooks.cpp)

target_link_libraries(triton_krackme PRIVATE LIEF::LIEF)
target_link_libraries(triton_krackme PRIVATE triton)
target_link_libraries(triton_krackme PRIVATE utils)
target_link_libraries(triton_krackme PRIVATE hooks)

install(TARGETS triton_krackme
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
