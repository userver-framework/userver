_userver_module_begin(
    NAME cctz
    VERSION 2.3
    DEBIAN_NAMES libcctz-dev
    FORMULA_NAMES cctz
    RPM_NAMES cctz-devel
    PACMAN_NAMES cctz
)

_userver_module_find_include(
    NAMES cctz/civil_time.h
)

_userver_module_find_library(
    NAMES cctz
)

_userver_module_end()
