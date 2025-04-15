_userver_module_begin(
    NAME SQLite3
    VERSION 3.46.1
    DEBIAN_NAMES libsqlite3-dev
    FORMULA_NAMES sqlite
    RPM_NAMES sqlite-devel
    PACMAN_NAMES sqlite
    PKG_NAMES sqlite3
    PKG_CONFIG_NAMES sqlite3
)

_userver_module_find_include(
    NAMES sqlite3/sqlite3.h sqlite3.h
    PATHS
    /usr/local/opt/sqlite/include
    /opt/homebrew/opt/sqlite/include
)

_userver_module_find_library(
    NAMES SQLite3
)

_userver_module_end()

if(NOT TARGET SQLite::SQLite3)
    add_library(SQLite::SQLite3 ALIAS SQLite3)
endif()
