include_guard(GLOBAL)

# find_package(OpenSSL)
# if(OpenSSL_FOUND)
#     return()
# endif()
 
include(DownloadUsingCPM)
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
if(FALSE)
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
endif()

cpmaddpackage(
     NAME OpenSSL
     URL https://github.com/openssl/openssl/releases/download/openssl-3.5.2/openssl-3.5.2.tar.gz
)
if(FALSE)
add_custom_command(
    OUTPUT
        ${CMAKE_BINARY_DIR}/_deps/openssl-build/libssl.a
        ${CMAKE_BINARY_DIR}/_deps/openssl-build/libcrypto.a
        ${CMAKE_BINARY_DIR}/openssl/usr/local/include/openssl/cms.h
    COMMAND
        ./config no-idea no-mdc2 no-rc5 no-zlib no-ssl3 enable-unit-test no-ssl3-method enable-rfc3779 enable-cms no-capieng
    COMMAND
	make -j8
    COMMAND
        make DESTDIR=${OPENSSL_INSTALL_DIR} install_sw
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/openssl-src/
)
endif()

execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/openssl/usr/local/include)

if(TRUE)
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
endif()


add_library(Crypto STATIC IMPORTED GLOBAL)
add_dependencies(Crypto OpenSSL)
add_dependencies(Crypto ${CMAKE_BINARY_DIR}/openssl/usr/local/include/openssl/cms.h)
set_property(TARGET Crypto PROPERTY IMPORTED_LOCATION
        ${CMAKE_BINARY_DIR}/_deps/openssl-build/libcrypto.a
    )
target_include_directories(Crypto INTERFACE ${CMAKE_BINARY_DIR}/openssl/usr/local/include)


add_library(OpenSSL::Crypto ALIAS Crypto)

add_library(SSL STATIC IMPORTED GLOBAL)
add_dependencies(SSL OpenSSL)
add_dependencies(SSL ${CMAKE_BINARY_DIR}/openssl/usr/local/include/openssl/cms.h)
set_property(TARGET SSL PROPERTY IMPORTED_LOCATION
        ${CMAKE_BINARY_DIR}/_deps/openssl-build/libssl.a
    )
target_include_directories(SSL INTERFACE ${CMAKE_BINARY_DIR}/openssl/usr/local/include)

add_library(OpenSSL::SSL ALIAS SSL)
