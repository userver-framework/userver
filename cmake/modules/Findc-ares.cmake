_userver_module_begin(
    NAME c-ares
    VERSION 1.16.0 # ares_getaddrinfo appeared only in 1.16
    DEBIAN_NAMES libc-ares-dev
    FORMULA_NAMES c-ares
    RPM_NAMES c-ares-devel
    PACMAN_NAMES c-ares
    PKG_CONFIG_NAMES libcares
    CPM_NAME c-ares
    CPM_VERSION 1.34.5
    CPM_URL https://github.com/c-ares/c-ares/archive/v1.34.5.tar.gz
    CPM_URL_HASH SHA256=dcd919635f01b7c8c9c2f5fb38063cd86500f7c6d4d32ecf4deff5e3497fb157
    CPM_OPTIONS "CARES_STATIC ON" "CARES_SHARED OFF" "CARES_INSTALL OFF" "CARES_BUILD_TOOLS OFF" "CARES_STATIC_PIC ON"
)

_userver_module_find_include(NAMES ares.h)

_userver_module_find_library(NAMES cares_static cares)

_userver_module_end()

if(c-ares_FOUND AND NOT TARGET c-ares::cares)
    # NOTE: intentionally not an ALIAS of the bare "c-ares" target. install(EXPORT)
    # resolves ALIAS targets to their underlying target, and the bare "c-ares"
    # IMPORTED target (created by _userver_module_end()) is not in any export set,
    # which makes CMake fail with "requires target 'c-ares' that is not in any
    # export set". A standalone namespaced IMPORTED target is treated by CMake as
    # coming from an external find_package() call (which userver-core-config.cmake
    # already performs for consumers) and is exempt from that check.
    add_library(c-ares::cares INTERFACE IMPORTED GLOBAL)
    set_target_properties(
        c-ares::cares PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${c-ares_INCLUDE_DIRS}"
                                  INTERFACE_LINK_LIBRARIES "${c-ares_LIBRARIES}"
    )
endif()
