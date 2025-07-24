_userver_module_begin(
    NAME
    fmt
    VERSION
    8.1.1
    DEBIAN_NAMES
    libfmt-dev
    FORMULA_NAMES
    fmt
    RPM_NAMES
    fmt-devel
    PACMAN_NAMES
    fmt
    PKG_CONFIG_NAMES
    fmt
)

_userver_module_find_include(NAMES fmt/format.h)

_userver_module_find_library(NAMES fmt)

_userver_module_end()

if(NOT TARGET fmt)
    add_library(fmt ALIAS fmt::fmt)
endif()
