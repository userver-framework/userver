cmake_minimum_required(VERSION 3.15)
project(LRUCacheDemo)

get_filename_component(MULTIINDEX_ROOT "../include" ABSOLUTE)
message(STATUS "Boost root: ${MULTIINDEX_ROOT}")

find_package(Boost QUIET)

if(Boost_FOUND)
    message(STATUS "Found system Boost: ${Boost_INCLUDE_DIRS}")
endif()

find_package(benchmark QUIET)

if(benchmark_FOUND)
    message(STATUS "Found Google Benchmark via find_package")
endif()

include_directories(${MULTIINDEX_ROOT})

add_executable(lru_cache_demo
    main.cpp
)

target_include_directories(lru_cache_demo PRIVATE ${Boost_INCLUDE_DIRS})
target_compile_features(lru_cache_demo PRIVATE cxx_std_20)

target_link_libraries(lru_cache_demo PRIVATE benchmark::benchmark)