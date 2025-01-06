_userver_module_begin(
    NAME LibEv
    DEBIAN_NAMES libev-dev
    FORMULA_NAMES libev
    RPM_NAMES libev-devel
    PACMAN_NAMES libev
)

_userver_module_find_include(
    NAMES ev.h
)

_userver_module_find_library(
    NAMES ev
)

_userver_module_end()
