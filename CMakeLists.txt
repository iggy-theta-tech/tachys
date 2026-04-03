cmake_minimum_required(VERSION 3.28)
project(tachys LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Google Test v1.14.0
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Google Benchmark v1.8.3
FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.8.3
    GIT_SHALLOW    TRUE
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

# fmt v10.2.1
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        10.2.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(fmt)

# simdjson v3.9.1
FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG        v3.9.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(simdjson)

# Boost 1.84.0 (header-only: Asio, Beast)
FetchContent_Declare(
    boost
    URL      https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
    URL_HASH SHA256=2e64e5d79a738d0fa6fb546c6e5c2bd28f88d268a2a080546f74e5ff98f29d0e
)
set(BOOST_INCLUDE_LIBRARIES asio beast system)
set(BOOST_ENABLE_CMAKE ON)
FetchContent_MakeAvailable(boost)

add_subdirectory(src/core)
add_subdirectory(src/infra)
add_subdirectory(src/feed)
add_subdirectory(src/book)
add_subdirectory(src/strategy)
add_subdirectory(src/oms)
add_subdirectory(src/risk)
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmarks)
add_subdirectory(exercises)
