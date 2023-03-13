if (TARGET libbenchmark)
    return()
endif()

macro(userver_add_gbench_subdirectory GBENCH_PATH_SUFFIX)
    set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "")
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "")
    set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "")
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "")
    add_subdirectory("${USERVER_ROOT_DIR}/${GBENCH_PATH_SUFFIX}" "${CMAKE_BINARY_DIR}/${GBENCH_PATH_SUFFIX}")
    target_compile_options(benchmark PRIVATE "-Wno-format-nonliteral")
    add_library(libbenchmark ALIAS benchmark)  # Unify link names

    add_library(UserverGBench ALIAS benchmark)
    set(UserverGBench_VERSION "1.6.1" CACHE STRING "Version of the gbench")
endmacro()

macro(userver_fetch_and_add_gbench_subdirectory)
    FetchContent_GetProperties(gbench_external_project)
    if (NOT gbench_external_project_POPULATED)
      message(STATUS "Downloading gbench from remote")
      FetchContent_Populate(gbench_external_project)
    endif()

    userver_add_gbench_subdirectory("third_party/gbench")
endmacro()

option(USERVER_DOWNLOAD_PACKAGE_GBENCH "Download and setup gbench if no gbench of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})
if (USERVER_DOWNLOAD_PACKAGE_GBENCH)
    find_package(UserverGBench)
else()
    find_package(UserverGBench REQUIRED)
endif()

if (UserverGBench_FOUND)
    if (NOT TARGET libbenchmark)
        add_library(libbenchmark ALIAS UserverGBench)  # Unify link names
    endif()
    return()
endif()

include(FetchContent)
FetchContent_Declare(
  gbench_external_project
  GIT_REPOSITORY https://github.com/google/benchmark.git
  TIMEOUT 10
  GIT_TAG v1.6.1
  SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/gbench
)
userver_fetch_and_add_gbench_subdirectory()
