_userver_module_begin(
    NAME libmariadb
    VERSION 3.0.3
    DEBIAN_NAMES libmariadb-dev
    FORMULA_NAMES mariadb
    PKG_CONFIG_NAMES mariadb
)

_userver_module_find_include(
    NAMES mysql.h
    PATH_SUFFIXES mariadb mysql
)

_userver_module_find_library(
    NAMES mariadb
)

_userver_module_end()

find_path(MYSQL_H NAMES mariadb/mysql.h)
if (MYSQL_H STREQUAL "MYSQL_H-NOTFOUND")
  set(USERVER_MYSQL_OLD_INCLUDE YES)
endif()
