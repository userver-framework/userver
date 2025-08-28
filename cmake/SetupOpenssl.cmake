include_guard(GLOBAL)

if(NOT OpenSSL_CPM)
  find_package(OpenSSL)
  if(OpenSSL_FOUND)
      return()
  endif()
endif()
set(OpenSSL_CPM TRUE CACHE BOOL "")
 
include(DownloadUsingCPM)

set(OPENSSL_INSTALL_DIR ${CMAKE_BINARY_DIR}/openssl)
execute_process(COMMAND mkdir -p ${OPENSSL_INSTALL_DIR}/usr/local/include)

# Flags are copied from Ubuntu's debian/rules
set(CONFIGURE_FLAGS no-idea no-mdc2 no-rc5 no-zlib no-ssl3 enable-unit-test no-ssl3-method enable-rfc3779 enable-cms no-capieng)
set(OPENSSL_VERSION 3.5.2)

cpmaddpackage(
     NAME OpenSSL
     URL https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz
     URL_HASH SHA512=db2c7a88bea432f96d867a98af15f850f371d4136c657338de93cb88a39a3578c025b5df7310e195a02fc715ad5a2422a319a44f0247c6a7e2ba8b36aad77651
)

# We use custom target to be able to set build dependency
# for *external* libcurl from CPM
add_custom_target(
    OpenSSL
        test -e ${OPENSSL_INSTALL_DIR}/.installed || ./config ${CONFIGURE_FLAGS}
    COMMAND
        test -e ${OPENSSL_INSTALL_DIR}/.installed || make -j8
    COMMAND
        test -e ${OPENSSL_INSTALL_DIR}/.installed || make DESTDIR=${OPENSSL_INSTALL_DIR} install_sw
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

# Light emulation of find_package(OpenSSL)
set(OpenSSL_FOUND TRUE CACHE BOOL "" FORCE)
set(OPENSSL_FOUND TRUE CACHE BOOL "" FORCE)
