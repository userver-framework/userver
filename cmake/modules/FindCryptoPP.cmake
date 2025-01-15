_userver_module_begin(
    NAME CryptoPP
    DEBIAN_NAMES libcrypto++-dev
    FORMULA_NAMES cryptopp
    RPM_NAMES cryptopp-devel
    PACMAN_NAMES crypto++
    PKG_CONFIG_NAMES libcrypto++
)

_userver_module_find_include(
    NAMES cryptopp/cryptlib.h
    PATH_SUFFIXES include
)

_userver_module_find_library(
    NAMES cryptopp cryptlib
    PATH_SUFFIXES lib
)

_userver_module_end()
