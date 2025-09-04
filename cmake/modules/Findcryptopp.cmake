_userver_module_begin(
    NAME
    cryptopp
    DEBIAN_NAMES
    libcrypto++-dev
    FORMULA_NAMES
    cryptopp
    RPM_NAMES
    cryptopp-devel
    PACMAN_NAMES
    crypto++
    PKG_CONFIG_NAMES
    libcrypto++
)

_userver_module_find_include(NAMES cryptopp/cryptlib.h PATH_SUFFIXES include)

_userver_module_find_library(NAMES cryptopp cryptlib PATH_SUFFIXES lib)

_userver_module_end()

if(NOT TARGET cryptopp::cryptopp)
    add_library(cryptopp::cryptopp ALIAS cryptopp)
endif()
