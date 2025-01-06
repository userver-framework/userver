_userver_module_begin(
    NAME UserverGBench
    VERSION 1.5  # libbenchmark_main.a appeared after 1.3
    DEBIAN_NAMES libbenchmark-dev
    FORMULA_NAMES google-benchmark
    RPM_NAMES google-benchmark-devel
    PACMAN_NAMES benchmark
    PKG_CONFIG_NAMES benchmark
)

_userver_module_find_include(
    NAMES benchmark/benchmark.h
)

_userver_module_find_library(
    NAMES benchmark_main
)

_userver_module_find_library(
    NAMES benchmark
)

_userver_module_end()
