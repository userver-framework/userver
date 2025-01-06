_userver_module_begin(
    NAME libzip
    DEBIAN_NAMES libbz2-dev
    FORMULA_NAMES libbz2
    RPM_NAMES libbz2-dev
    PACMAN_NAMES bzip2
)

_userver_module_find_include(
    NAMES bzlib.h
    PATH_SUFFIXES include
)

_userver_module_find_library(
    NAMES bz2 libbz2
    PATH_SUFFIXES lib
)

_userver_module_end()
