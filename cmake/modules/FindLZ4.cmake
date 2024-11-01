_userver_module_begin(
    NAME LZ4
    VERSION 1.9.2
    DEBIAN_NAMES liblz4-dev
    FORMULA_NAMES lz4
    PKG_CONFIG_NAMES liblz4
)

_userver_module_find_include(
    NAMES lz4.h
)

_userver_module_find_library(
    NAMES liblz4.a
)

_userver_module_end()

add_library(LZ4::LZ4 ALIAS LZ4)
