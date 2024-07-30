_userver_module_begin(
    NAME libintl
)

_userver_module_find_include(
    NAMES stdio.h
)

_userver_module_find_library(
    NAMES libintl.a  # FreeBSD
    PATHS /usr/local/lib
)

_userver_module_end()
