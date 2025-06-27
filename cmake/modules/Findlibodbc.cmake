_userver_module_begin(
    NAME
    libodbc
    VERSION
    2.3.6
    DEBIAN_NAMES
    unixodbc-dev
    FORMULA_NAMES
    unixodbc
    PKG_CONFIG_NAMES
    unixodbc
)

_userver_module_find_include(NAMES sql.h sqlext.h PATH_SUFFIXES unixodbc odbc)

_userver_module_find_library(NAMES odbc unixodbc)

_userver_module_end()
