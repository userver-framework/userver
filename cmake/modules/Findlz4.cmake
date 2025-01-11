_userver_module_begin(
    NAME lz4
    VERSION 1.9.2
    DEBIAN_NAMES liblz4-dev
    FORMULA_NAMES lz4
    PKG_CONFIG_NAMES liblz4
)

_userver_module_find_include(
    NAMES lz4.h
)

_userver_module_find_library(
    NAMES lz4
)

_userver_module_end()

if(NOT TARGET lz4::lz4)
  add_library(lz4::lz4 ALIAS lz4)
endif()
