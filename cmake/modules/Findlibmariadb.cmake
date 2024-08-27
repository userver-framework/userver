_userver_module_begin(
    NAME libmariadb
    VERSION 3.0.3
    DEBIAN_NAMES libmariadb-dev
)

_userver_module_find_include(
    NAMES mariadb/mysql.h
)

_userver_module_find_library(
    NAMES mariadb
    PATH_SUFFIXES lib
)

_userver_module_end()
