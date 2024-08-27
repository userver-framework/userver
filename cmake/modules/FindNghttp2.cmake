_userver_module_begin(
    NAME Nghttp2
    DEBIAN_NAMES libnghttp2-dev
    FORMULA_NAMES nghttp2
    PACMAN_NAMES libnghttp2
)

_userver_module_find_include(
    NAMES nghttp2/nghttp2.h
)

_userver_module_find_library(
    NAMES nghttp2
)

_userver_module_end()
