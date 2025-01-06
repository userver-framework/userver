_userver_module_begin(
    NAME mongoc
    VERSION 1.16.0
    DEBIAN_NAMES libmongoc-dev
    FORMULA_NAMES mongo-c-driver
    RPM_NAMES mongo-c-driver-devel
    PACMAN_NAMES mongo-c-driver
)

_userver_module_find_include(
    NAMES mongoc/mongoc.h
    PATHS
    /usr/include/libmongoc-1.0
    /usr/local/opt/mongo-c/include/libmongoc-1.0
)

_userver_module_find_library(
    NAMES mongoc mongoc-1.0
)

_userver_module_end()
