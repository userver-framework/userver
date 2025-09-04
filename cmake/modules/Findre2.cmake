if(TARGET re2::re2)
    return()
endif()

_userver_module_begin(
    NAME
    re2
    VERSION
    20180101
    DEBIAN_NAMES
    libre2-dev
    FORMULA_NAMES
    re2
    RPM_NAMES
    re2
    PACMAN_NAMES
    re2
    PKG_CONFIG_NAMES
    re2
)

_userver_module_find_include(NAMES re2/re2.h)

_userver_module_find_library(NAMES re2)

_userver_module_end()

if(NOT TARGET re2::re2)
    add_library(re2::re2 ALIAS re2)
endif()
