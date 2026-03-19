cmake_minimum_required(VERSION 3.15...3.28)

project(ROCpdBenchmark
    VERSION 1.0.0
    DESCRIPTION "ROCpd SQLite schema benchmarking with real data"
    LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(SQLite3 REQUIRED)

# Fetch Google Benchmark
include(FetchContent)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable gtest tests" FORCE)
FetchContent_MakeAvailable(benchmark)

# Create real data benchmark (original vs combined schema)
add_executable(benchmark_schema
    main.cpp
    source/types.cpp
    source/data_loader.cpp
    source/db_helpers.cpp
    source/schema_original.cpp
    source/schema_combined.cpp
    source/benchmarks.cpp
)
target_link_libraries(benchmark_schema
    PRIVATE
        SQLite::SQLite3
        benchmark::benchmark
)
target_compile_options(benchmark_schema PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Copy schema files to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/schemas/rocpd_tables.sql
               ${CMAKE_CURRENT_BINARY_DIR}/rocpd_tables.sql
               COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/schemas/rocpd_tables_combined.sql
               ${CMAKE_CURRENT_BINARY_DIR}/rocpd_tables_combined.sql
               COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/schemas/rocpd_tables_indexed.sql
               ${CMAKE_CURRENT_BINARY_DIR}/rocpd_tables_indexed.sql
               COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/schemas/rocpd_tables_combined_indexed.sql
               ${CMAKE_CURRENT_BINARY_DIR}/rocpd_tables_combined_indexed.sql
               COPYONLY)

# Copy real database to build directory for benchmarking
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/benchmark_data/rocpd-7389.db
               ${CMAKE_CURRENT_BINARY_DIR}/benchmark_data/rocpd-7389.db
               COPYONLY)

# Print build information
message(STATUS "Building ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
