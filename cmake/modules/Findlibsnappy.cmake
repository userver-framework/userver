_userver_module_begin(
    NAME libsnappy
    DEBIAN_NAMES libsnappy-dev
    FORMULA_NAMES snappy
    RPM_NAMES libsnappy-dev
    PACMAN_NAMES snappy
    CPM_NAME snappy
    CPM_VERSION 1.2.1
    CPM_URL https://github.com/google/snappy/archive/1.2.1.tar.gz
    CPM_URL_HASH SHA256=736aeb64d86566d2236ddffa2865ee5d7a82d26c9016b36218fcc27ea4f09f86
    CPM_OPTIONS "SNAPPY_BUILD_TESTS OFF" "SNAPPY_BUILD_BENCHMARKS OFF" "SNAPPY_INSTALL OFF"
)

_userver_module_find_include(NAMES snappy-c.h snappy-sinksource.h snappy-stubs-public.h snappy.h PATH_SUFFIXES include)

_userver_module_find_library(NAMES snappy PATH_SUFFIXES lib)

_userver_module_end()

if(libsnappy_ADDED)
    if(TARGET snappy)
        set(_userver_snappy_real snappy)
    else()
        message(FATAL_ERROR "snappy cmake target not found after CPM download, don't know how to link")
    endif()
    if(NOT TARGET libsnappy)
        add_library(libsnappy ALIAS ${_userver_snappy_real})
    endif()
    if(NOT TARGET Snappy::snappy)
        add_library(Snappy::snappy ALIAS ${_userver_snappy_real})
        add_library(Snappy::snappy-static ALIAS ${_userver_snappy_real})
    endif()
    unset(_userver_snappy_real)
else()
    if(NOT TARGET Snappy::snappy)
        add_library(Snappy::snappy ALIAS libsnappy)
        add_library(Snappy::snappy-static ALIAS libsnappy)
    endif()
endif()
