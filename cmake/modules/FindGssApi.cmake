_userver_module_begin(
    NAME GssApi
    DEBIAN_NAMES libkrb5-dev
    FORMULA_NAMES krb5
    PACMAN_NAMES krb5
)

_userver_module_find_include(
    NAMES gssapi.h
    PATH_SUFFIXES gssapi
)

_userver_module_find_library(
    NAMES gssapi_krb5 gssapi
    PATH_SUFFIXES gssapi
)

_userver_module_end()
