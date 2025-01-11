_userver_module_begin(
    NAME libzstd
    DEBIAN_NAMES libzstd-dev
    FORMULA_NAMES zstd
    RPM_NAMES libzstd-dev
    PACMAN_NAMES zstd
)

_userver_module_find_include(
    NAMES
    zdict.h
    zstd.h
    zstd_errors.h
    PATH_SUFFIXES include
)

_userver_module_find_library(
    NAMES zstd
    PATH_SUFFIXES lib
)

_userver_module_end()

if(NOT TARGET zstd::zstd)
  add_library(zstd::zstd ALIAS libzstd)
endif()
add_library(zstd ALIAS libzstd)
