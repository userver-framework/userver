include_guard(GLOBAL)

find_package(OpenSSL)
if(OpenSSL_FOUND)
    return()
endif()
 
include(DownloadUsingCPM)
include(ExternalProject)

set(OPENSSL_INSTALL_DIR ${CMAKE_BINARY_DIR}/openssl)
execute_process(COMMAND mkdir -p ${OPENSSL_INSTALL_DIR}/usr/local/include)

cpmaddpackage(
     NAME OpenSSL
     URL https://github.com/openssl/openssl/releases/download/openssl-3.5.2/openssl-3.5.2.tar.gz
     URL_HASH SHA512=123
)


add_custom_target(
    OpenSSL
        # Flags are copied from Ubuntu's debian/rules
        test -e ${OPENSSL_INSTALL_DIR}/.installed || 
        ./config no-idea no-mdc2 no-rc5 no-zlib no-ssl3 enable-unit-test no-ssl3-method enable-rfc3779 enable-cms no-capieng
    COMMAND
        test -e ${OPENSSL_INSTALL_DIR}/.installed || 
	make -j8
    COMMAND
        test -e ${OPENSSL_INSTALL_DIR}/.installed || 
        make DESTDIR=${OPENSSL_INSTALL_DIR} install_sw
    COMMAND
        touch ${OPENSSL_INSTALL_DIR}/.installed
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/openssl-src/
    COMMENT "Compiling OpenSSL library"
)


add_library(Crypto STATIC IMPORTED GLOBAL)
add_dependencies(Crypto OpenSSL)
set_property(TARGET Crypto PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/_deps/openssl-build/libcrypto.a)
target_include_directories(Crypto INTERFACE ${CMAKE_BINARY_DIR}/openssl/usr/local/include)


add_library(SSL STATIC IMPORTED GLOBAL)
add_dependencies(SSL OpenSSL)
set_property(TARGET SSL PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/_deps/openssl-build/libssl.a)
target_include_directories(SSL INTERFACE ${CMAKE_BINARY_DIR}/openssl/usr/local/include)

add_library(OpenSSL::Crypto ALIAS Crypto)
add_library(OpenSSL::SSL ALIAS SSL)
