_userver_module_begin(
    NAME RdKafka
    VERSION 2.3.0
    DEBIAN_NAMES librdkafka-dev
    FORMULA_NAMES librdkafka
    PACMAN_NAMES librdkafka
    PKG_CONFIG_NAMES rdkafka
)

_userver_module_find_include(
    NAMES librdkafka/rdkafka.h
)

_userver_module_find_library(
    NAMES librdkafka.a
)

_userver_module_end()
