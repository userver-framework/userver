if (TARGET libbenchmark)
    return()
endif()

option(USERVER_DOWNLOAD_PACKAGE_GBENCH "Download and setup gbench if no gbench of matching version was found" ${USERVER_DOWNLOAD_PACKAGES})

if (NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
    if (USERVER_DOWNLOAD_PACKAGE_GBENCH)
        find_package(UserverGBench QUIET)
    else()
        find_package(UserverGBench REQUIRED)
    endif()

    if (UserverGBench_FOUND)
        if (NOT TARGET libbenchmark)
            add_library(libbenchmark ALIAS UserverGBench)  # Unify link names
        endif()
        return()
    endif()
endif()

include(DownloadUsingCPM)
CPMAddPackage(
    NAME benchmark
    VERSION 1.6.1
    GITHUB_REPOSITORY google/benchmark
    OPTIONS
    "BENCHMARK_ENABLE_TESTING OFF"
    "BENCHMARK_ENABLE_WERROR OFF"
    "BENCHMARK_ENABLE_INSTALL OFF"
    "BENCHMARK_INSTALL_DOCS OFF"
    "BENCHMARK_ENABLE_GTEST_TESTS OFF"
)

target_compile_options(benchmark PRIVATE "-Wno-format-nonliteral")
add_library(libbenchmark ALIAS benchmark)  # Unify link names
