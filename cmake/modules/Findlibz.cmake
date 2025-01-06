_userver_module_begin(
    NAME libz
    DEBIAN_NAMES zlib1g-dev
    FORMULA_NAMES zlib
    RPM_NAMES zlib1g-dev
    PACMAN_NAMES zlib
)

_userver_module_find_include(
    NAMES zlib.h zconf.h
    PATH_SUFFIXES include
)

_userver_module_find_library(
    NAMES zlib1g z
    PATH_SUFFIXES lib
)

_userver_module_end()
