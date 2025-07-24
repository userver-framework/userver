_userver_module_begin(
    NAME
    bson
    VERSION
    1.16.0
    DEBIAN_NAMES
    libmongoc-dev
    FORMULA_NAMES
    mongo-c-driver@1
    RPM_NAMES
    mongo-c-driver-devel
    PACMAN_NAMES
    mongo-c-driver
    PKG_CONFIG_NAMES
    libbson-1.0
)

_userver_module_find_include(
    NAMES bson/bson.h PATHS /usr/include/libbson-1.0 /usr/local/opt/mongo-c-driver/include/libbson-1.0
    /opt/homebrew/opt/mongo-c-driver@1/include/libbson-1.0
)

_userver_module_find_library(
    NAMES
    bson-1.0
    bson
    PATHS
    /usr/include/libbson-1.0
    /usr/local/opt/mongo-c-driver/include/libbson-1.0
    /opt/homebrew/opt/mongo-c-driver@1/lib
)

_userver_module_end()

if(NOT TARGET mongo::bson_static)
    add_library(mongo::bson_static ALIAS bson)
endif()
