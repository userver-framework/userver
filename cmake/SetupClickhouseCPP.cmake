set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

option(USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP "Download and setup clickhouse-cpp" ${USERVER_DOWNLOAD_PACKAGES})

if (USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP)
  find_package(clickhouse-cpp)
else()
  find_package(clickhouse-cpp REQUIRED)
endif()

if (clickhouse-cpp_FOUND)
  return()
endif()

set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)

include(FetchContent)
FetchContent_Declare(
  clickhouse-cpp_external_project
  GIT_REPOSITORY https://github.com/ClickHouse/clickhouse-cpp.git
  TIMEOUT 10
  GIT_TAG v2.3.0
  SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/clickhouse-cpp
)
FetchContent_GetProperties(clickhouse-cpp_external_project)
if(NOT clickhouse-cpp_external_project_POPULATED)
  message(STATUS "Downloading clickhouse-cpp from remote")
  FetchContent_Populate(clickhouse-cpp_external_project)
endif()

add_subdirectory(${USERVER_ROOT_DIR}/third_party/clickhouse-cpp "${CMAKE_BINARY_DIR}/third_party/clickhouse-cpp")
add_library(clickhouse-cpp ALIAS clickhouse-cpp-lib-static)
set(clickhouse-cpp_VERSION "2.3.0" CACHE STRING "Version of the clickhouse-cpp")

target_compile_options(absl-lib PUBLIC -Wno-pedantic)
