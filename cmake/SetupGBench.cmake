option(USERVER_FEATURE_GBENCH_DOWNLOAD "Download and setup gbench if no gbench of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})
if (USERVER_FEATURE_GBENCH_DOWNLOAD)
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

FetchContent_GetProperties(gbench_external_project)
if (NOT gbench_external_project_POPULATED)
  message(STATUS "Downloading gbench from remote")
  FetchContent_Populate(gbench_external_project)
endif()

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
add_subdirectory(${USERVER_ROOT_DIR}/third_party/gbench "${CMAKE_BINARY_DIR}/third_party/gbench")
add_library(libbenchmark ALIAS benchmark)  # Unify link names

add_library(UserverGBench ALIAS benchmark)
set(UserverGBench_VERSION "1.6.1" CACHE STRING "Version of the gbench")
