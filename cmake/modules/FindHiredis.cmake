_userver_module_begin(
    NAME Hiredis
    VERSION 0.13.3
    DEBIAN_NAMES libhiredis-dev
    FORMULA_NAMES hiredis
    RPM_NAMES hiredis-devel
    PACMAN_NAMES hiredis
    PKG_NAMES hiredis
    PKG_CONFIG_NAMES hiredis
)

_userver_module_find_include(
    NAMES hiredis/hiredis.h
)

_userver_module_find_library(
    NAMES hiredis
)

_userver_module_end()
