cmake_minimum_required(VERSION 3.5)

project(triton_krackme LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

find_package(triton REQUIRED CONFIG)
find_package(LIEF REQUIRED CONFIG)
link_libraries(${TRITON_LIBRARIES})
link_libraries(${LIEF_LIBRARIES})

include_directories(${TRITON_INCLUDE_DIRS})
include_directories(${LIEF_INCLUDE_DIR})
# find_package(LIEF REQUIRED COMPONENTS STATIC) find_package(triton REQUIRED
# CONFIG)

add_executable(triton_krackme main.cpp utils.hpp routines.hpp ttexplore.hpp)
add_library(utils STATIC utils.cpp)
add_library(ttexplore STATIC ttexplore.cpp routines.cpp)

target_link_libraries(triton_krackme PRIVATE utils)
target_link_libraries(triton_krackme PRIVATE ttexplore)

install(TARGETS triton_krackme LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
