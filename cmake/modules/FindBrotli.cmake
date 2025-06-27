_userver_module_begin(
    NAME
    Brotli
    DEBIAN_NAMES
    libbrotli-dev
    FORMULA_NAMES
    brotli
    PACMAN_NAMES
    brotli
)

_userver_module_find_include(NAMES brotli/decode.h)

_userver_module_find_include(NAMES brotli/encode.h)

_userver_module_find_library(NAMES brotlidec)

_userver_module_find_library(NAMES brotlienc)

_userver_module_end()

if(NOT TARGET Brotli::dec)
    add_library(Brotli::dec ALIAS brotlidec)
endif()
if(NOT TARGET Brotli::enc)
    add_library(Brotli::enc ALIAS brotlienc)
endif()
