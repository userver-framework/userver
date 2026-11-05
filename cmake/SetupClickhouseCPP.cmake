# @ingroup download
option(USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP "Download and setup clickhouse-cpp" ${USERVER_DOWNLOAD_PACKAGES})

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    find_package(lz4 REQUIRED)
    include(SetupAbseil)
endif()

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
    if(USERVER_DOWNLOAD_PACKAGE_CLICKHOUSECPP)
        find_package(clickhouse-cpp QUIET)
    else()
        find_package(clickhouse-cpp REQUIRED)
    endif()

    if(clickhouse-cpp_FOUND)
        if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
            target_link_libraries(clickhouse-cpp INTERFACE lz4::lz4 absl::int128)
        endif()
        return()
    endif()
endif()

include(DownloadUsingCPM)

if(USERVER_INSTALL)
    message(
        FATAL_ERROR
            "userver-clickhouse cannot be built with USERVER_INSTALL=ON: clickhouse-cpp "
            "was not found as a system package and would be downloaded via CPM, which "
            "cannot be added to a CMake install export set.\n"
            "Install a system clickhouse-cpp (so find_package(clickhouse-cpp) succeeds), "
            "or disable this component for install builds: -DUSERVER_FEATURE_CLICKHOUSE=OFF."
    )
endif()

include(SetupAbseil)

cpmaddpackage(
    NAME clickhouse-cpp
    VERSION 2.6.2
    URL https://github.com/ClickHouse/clickhouse-cpp/archive/001025cd72d904104c15657e85e0fb6b0ec58e14.tar.gz
    URL_HASH SHA256=d198fd39edd8d0e662bd722bdb8bed05c047b5508316b64e8d03b8b92b5f199f
    SYSTEM
    OPTIONS "WITH_SYSTEM_ABSEIL ON" "WITH_SYSTEM_LZ4 ON" "WITH_SYSTEM_ZSTD ON" "DEBUG_DEPENDENCIES OFF"
)

if(NOT TARGET clickhouse-cpp-lib::clickhouse-cpp-lib)
    add_library(clickhouse-cpp-lib::clickhouse-cpp-lib ALIAS clickhouse-cpp-lib)
endif()
