_userver_module_begin(
    NAME c-ares
    VERSION 1.16.0  # ares_getaddrinfo appeared only in 1.16
    DEBIAN_NAMES libc-ares-dev
    FORMULA_NAMES c-ares
    RPM_NAMES c-ares-devel
    PACMAN_NAMES c-ares
    PKG_CONFIG_NAMES libcares
)

_userver_module_find_include(
    NAMES ares.h
)

_userver_module_find_library(
    NAMES cares_static cares
)

_userver_module_end()
