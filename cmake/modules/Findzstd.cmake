_userver_module_begin(
    NAME zstd
    DEBIAN_NAMES libzstd-dev
    FORMULA_NAMES zstd
    RPM_NAMES libzstd-dev
    PACMAN_NAMES zstd
                 # TODO: CPM
)

_userver_module_find_include(NAMES zdict.h zstd.h zstd_errors.h PATH_SUFFIXES include)

_userver_module_find_library(NAMES zstd PATH_SUFFIXES lib)

_userver_module_end()

if(NOT TARGET zstd::zstd)
    if(TARGET libzstd_static)
        add_library(zstd::zstd ALIAS libzstd_static)
    else()
        add_library(zstd::zstd ALIAS zstd)
    endif()
endif()
if(NOT TARGET ZSTD::ZSTD)
    add_library(ZSTD::ZSTD ALIAS zstd)
endif()
