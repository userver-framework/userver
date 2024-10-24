if (TARGET benchmark::benchmark)
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
        if (NOT TARGET benchmark::benchmark)
            add_library(benchmark::benchmark ALIAS UserverGBench)  # Unify link names
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

include(UserverCxxCompileOptionsIfSupported)
userver_target_cxx_compile_options_if_supported(benchmark PRIVATE "-Wno-format-nonliteral")
if (NOT TARGET benchmark::benchmark)
    add_library(benchmark::benchmark ALIAS benchmark)  # Unify link names
endif()
