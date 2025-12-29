_userver_module_begin(
    NAME c-ares
    VERSION 1.16.0 # ares_getaddrinfo appeared only in 1.16
    DEBIAN_NAMES libc-ares-dev
    FORMULA_NAMES c-ares
    RPM_NAMES c-ares-devel
    PACMAN_NAMES c-ares
    PKG_CONFIG_NAMES libcares
    CPM_NAME c-ares
    CPM_GITHUB_REPOSITORY c-ares/c-ares
    CPM_VERSION 1.34.5
    CPM_OPTIONS "CARES_STATIC ON" "CARES_SHARED OFF" "CARES_INSTALL OFF" "CARES_BUILD_TOOLS OFF" "CARES_STATIC_PIC ON"
)

_userver_module_find_include(NAMES ares.h)

_userver_module_find_library(NAMES cares_static cares)

_userver_module_end()

if(c-ares_FOUND AND NOT TARGET c-ares::cares)
    add_library(c-ares::cares ALIAS c-ares)
endif()
