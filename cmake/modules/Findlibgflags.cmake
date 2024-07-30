_userver_module_begin(
    NAME libgflags
    DEBIAN_NAMES libgflags-dev
    FORMULA_NAMES gflags
    RPM_NAMES libgflags-dev
    PACMAN_NAMES gflags
)

_userver_module_find_include(
    NAMES
    gflags/gflags.h
    gflags/gflags_completions.h
    gflags/gflags_declare.h
    gflags/gflags_gflags.h
    PATH_SUFFIXES include
)

_userver_module_find_library(
    NAMES gflags
    PATH_SUFFIXES lib
)

_userver_module_end()
