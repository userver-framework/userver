_userver_module_begin(
    NAME bson
    VERSION 1.16.0
    DEBIAN_NAMES libmongoc-dev
    FORMULA_NAMES mongo-c-driver
    RPM_NAMES mongo-c-driver-devel
    PACMAN_NAMES mongo-c-driver
)

_userver_module_find_include(
    NAMES bson/bson.h
    PATHS
    /usr/include/libbson-1.0
    /usr/local/opt/mongo-c-driver/include/libbson-1.0
)

_userver_module_find_library(
    NAMES bson bson-1.0
)

_userver_module_end()
