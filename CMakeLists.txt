cmake_minimum_required(VERSION 3.25)
project(numstore)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

# ===================================
# Find Packages
find_package(OpenMP REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW3F REQUIRED fftw3f)

# ===================================
# Set default Build type to debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -ggdb")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -ggdb")

# ===================================
# Build libnumstore
set(NS_LIB_DIR ${CMAKE_SOURCE_DIR}/libs/numstore)
file(GLOB_RECURSE NS_LIB_SRCS "${NS_LIB_DIR}/*.cpp")
add_library(numstore ${NS_LIB_SRCS})

target_include_directories(numstore PUBLIC include)
target_include_directories(numstore PRIVATE ${FFTW3F_INCLUDE_DIRS})
target_link_libraries(numstore PRIVATE ${FFTW3F_LIBRARIES} OpenMP::OpenMP_C m)

# ===================================
# Build executables
add_executable(main apps/main.cpp)
target_include_directories(main PUBLIC include)
target_link_libraries(main PUBLIC numstore)

add_executable(test apps/test.cpp)
target_include_directories(test PUBLIC include)
target_link_libraries(
  test PRIVATE -Wl,--whole-archive numstore -Wl,--no-whole-archive
)# Link all functions in numstore to get constructor functions
