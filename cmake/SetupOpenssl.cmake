include_guard(GLOBAL)

# find_package(OpenSSL)
# if(OpenSSL_FOUND)
#     return()
# endif()
 
# include(DownloadUsingCPM)
include(ExternalProject)

# cpmaddpackage(
#     URL https://github.com/openssl/openssl/releases/download/openssl-3.5.2/openssl-3.5.2.tar.gz
#     NAME OpenSSL
#     DOWNLOAD_ONLY
#     # GIT_TAG openssl-3.5.2
#     # CONFIGURE_COMMAND ./config ${CONFIGURE_OPENSSL_PARAMS} "no-cast no-md2 no-md4 no-mdc2 no-rc4 no-rc5 no-engine no-idea no-mdc2 no-rc5 no-camellia no-ssl3 no-heartbeats no-gost no-deprecated no-capieng no-comp no-dtls no-psk no-srp no-dso no-dsa no-rc2 no-des"
#     # EXCLUDE_FROM_ALL
# )
set(OPENSSL_INSTALL_DIR ${CMAKE_BINARY_DIR}/openssl)
externalproject_add(OpenSSL
	# URL https://github.com/openssl/openssl/releases/download/openssl-3.5.2/openssl-3.5.2.tar.gz
	# URL_HASH MD5=890fc59f86fc21b5e4d1c031a698dbde
	URL https://github.com/openssl/openssl/releases/download/openssl-3.0.2/openssl-3.0.2.tar.gz
	URL_HASH MD5=7f9d43bb7a1e742722cf6d6f40531462
    SOURCE_DIR ${CMAKE_BINARY_DIR}/openssl-src
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND cd <SOURCE_DIR> && ./config no-cast no-md2 no-md4 no-mdc2 no-rc4 no-rc5 no-engine no-idea no-mdc2 no-rc5 no-camellia no-ssl3 no-heartbeats no-gost no-deprecated no-capieng no-comp no-dtls no-psk no-srp no-dso no-dsa no-rc2 no-des

    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    # BUILD_COMMAND cd <SOURCE_DIR> && make -j8
    # INSTALL_COMMAND cd <SOURCE_DIR> && make DESTDIR=${OPENSSL_INSTALL_DIR} install_sw
)
ExternalProject_Add_Step(
	OpenSSL
	build_static
	COMMAND make -j8 && make DESTDIR=${OPENSSL_INSTALL_DIR} install_sw
	DEPENDS OpenSSL
	BYPRODUCTS
	    ${OPENSSL_INSTALL_DIR}/usr/local/lib/libcrypto.a
	    ${OPENSSL_INSTALL_DIR}/usr/local/lib/libssl.a
	EXCLUDE_FROM_MAIN 1
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/openssl-src
)
ExternalProject_Add_StepTargets(OpenSSL build_static)

add_library(Crypto STATIC IMPORTED)
# set_property(TARGET OpenSSL::Crypto PROPERTY IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/usr/local/lib/libcrypto.a)
set_property(TARGET Crypto PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/openssl-src/libcrypto.a)
target_include_directories(Crypto INTERFACE ${CMAKE_BINARY_DIR}/openssl-src/include)
add_dependencies(Crypto OpenSSL-build_static)

add_library(OpenSSL::Crypto ALIAS Crypto)

add_library(SSL STATIC IMPORTED)
# set_property(TARGET OpenSSL::SSL PROPERTY IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/usr/local/lib/libssl.a)
set_property(TARGET SSL PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/openssl-src/libssl.a)
add_dependencies(SSL OpenSSL-build_static)
target_include_directories(SSL INTERFACE ${CMAKE_BINARY_DIR}/openssl-src/include)

add_library(OpenSSL::SSL ALIAS SSL)
