_userver_module_begin(
    NAME
    mongoc
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
    libmongoc-1.0
)

_userver_module_find_include(
    NAMES mongoc/mongoc.h PATHS /usr/include/libmongoc-1.0 /usr/local/opt/mongo-c/include/libmongoc-1.0
    /opt/homebrew/opt/mongo-c-driver@1/include/libmongoc-1.0
)

_userver_module_find_library(
    NAMES
    mongoc-1.0
    mongoc
    PATHS
    /usr/include/libmongoc-1.0
    /usr/local/opt/mongo-c/include/libmongoc-1.0
    /opt/homebrew/opt/mongo-c-driver@1/lib
)

_userver_module_end()

if(NOT TARGET mongo::mongoc_static)
    add_library(mongo::mongoc_static ALIAS mongoc)
endif()
