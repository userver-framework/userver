_userver_module_begin(
    NAME clickhouse-cpp
)

_userver_module_find_include(
    NAMES clickhouse/block.h
    PATH_SUFFIXES
    clickhouse-cpp
    yandex/clickhouse-cpp
)

_userver_module_find_library(
    NAMES libclickhouse-cpp-lib.so
)

_userver_module_end()
