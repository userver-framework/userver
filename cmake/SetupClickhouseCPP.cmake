option(USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP "Download and setup clickhouse-cpp" ${USERVER_DOWNLOAD_PACKAGES})

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  if (USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP)
    find_package(clickhouse-cpp QUIET)
  else()
    find_package(clickhouse-cpp REQUIRED)
  endif()

  if (clickhouse-cpp_FOUND)
    return()
  endif()
endif()

include(DownloadUsingCPM)
include(SetupAbseil)

CPMAddPackage(
    NAME clickhouse-cpp
    VERSION 2.4.0
    GITHUB_REPOSITORY ClickHouse/clickhouse-cpp
    SYSTEM
    OPTIONS
    "WITH_SYSTEM_ABSEIL ON"
)

add_library(clickhouse-cpp ALIAS clickhouse-cpp-lib)
target_compile_options(clickhouse-cpp-lib PRIVATE -Wno-pedantic)
