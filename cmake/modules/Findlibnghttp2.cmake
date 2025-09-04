_userver_module_begin(
    NAME
    libnghttp2
    DEBIAN_NAMES
    libnghttp2-dev
    FORMULA_NAMES
    nghttp2
    PACMAN_NAMES
    libnghttp2
)

_userver_module_find_include(NAMES nghttp2/nghttp2.h)

_userver_module_find_library(NAMES nghttp2)

_userver_module_end()

if(NOT TARGET libnghttp2::nghttp2)
    add_library(libnghttp2::nghttp2 ALIAS libnghttp2)
endif()
