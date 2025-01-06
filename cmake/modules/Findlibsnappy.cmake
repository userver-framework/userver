_userver_module_begin(
    NAME libsnappy
    DEBIAN_NAMES libsnappy-dev
    FORMULA_NAMES snappy
    RPM_NAMES libsnappy-dev
    PACMAN_NAMES snappy
)

_userver_module_find_include(
    NAMES
    snappy-c.h
    snappy-sinksource.h
    snappy-stubs-public.h
    snappy.h
    PATH_SUFFIXES include
)

_userver_module_find_library(
    NAMES snappy
    PATH_SUFFIXES lib
)

_userver_module_end()
