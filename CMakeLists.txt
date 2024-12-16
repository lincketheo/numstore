cmake_minimum_required(VERSION 3.25)
project(numstore)

set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Ensure debug flags
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -ggdb")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -ggdb")

set(NS_LIB_DIR ${CMAKE_SOURCE_DIR}/libs/numstore)
file(GLOB_RECURSE NS_LIB_SRCS "${NS_LIB_DIR}/*.cpp")
add_library(numstore ${NS_LIB_SRCS})

target_include_directories(numstore PUBLIC include)

add_executable(main main.cpp)
add_executable(test test.cpp)

target_include_directories(main PUBLIC include)
target_link_libraries(main numstore)

target_include_directories(test PUBLIC include)
target_link_libraries(test PRIVATE -Wl,--whole-archive numstore -Wl,--no-whole-archive)

