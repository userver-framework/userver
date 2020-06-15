if(APPLE)
# Apple has no yandex-taxi's curl, fall back to libcurl
set(CURL_YANDEX_ENABLED_DEFAULT OFF)
set(CURL_CARES_ENABLED_DEFAULT OFF)
else(APPLE)
set(CURL_YANDEX_ENABLED_DEFAULT ON)
set(CURL_CARES_ENABLED_DEFAULT ON)
endif(APPLE)

set(CURL_YANDEX_ENABLED ${CURL_YANDEX_ENABLED_DEFAULT} CACHE BOOL "Whether to use libyandex-taxi-curl")
set(CURL_CARES_ENABLED ${CURL_CARES_ENABLED_DEFAULT} CACHE BOOL "Whether to use libyandex-taxi-cares")
message(STATUS "Using libyandex-taxi-curl status: ${CURL_YANDEX_ENABLED}")

if(NOT CURL_YANDEX_ENABLED)

find_package(CURL REQUIRED)
set(LIBCURL_YANDEX_LIBRARIES ${CURL_LIBRARIES})
set(LIBCURL_YANDEX_INCLUDE_DIR ${CURL_INCLUDE_DIRS})
set(LIBCURL_YANDEX_DEFINITIONS "")
find_package_handle_standard_args(CURLYANDEX LIBCURL_YANDEX_LIBRARIES LIBCURL_YANDEX_INCLUDE_DIR)

else(NOT CURL_YANDEX_ENABLED)

find_path(
  LIBCURL_YANDEX_INCLUDE_DIR curl/curl.h
  NO_DEFAULT_PATH
  PATHS
    "/usr/include/libyandex-taxi-curl/")
find_library(LIBCURL_YANDEX yandex-taxi-curl)

# We have to link against all these libraries as the static libyandex-taxi-curl.a
# doesn't store information about its dependencies :(
find_library(LIBGCRYPT gcrypt)
if(NOT LIBGCRYPT)
  message(FATAL_ERROR "Cannot find library gcrypt, try to install libgcrypt11-dev.")
endif()
find_library(LIBGSSAPI_LIBRARY gssapi_krb5)
if(NOT LIBGSSAPI_LIBRARY)
  message(FATAL_ERROR "Cannot find library gssapi_krb5, try to install libkrb5-dev.")
endif()
find_library(LIBKRB5_LIBRARY krb5)
if(NOT LIBKRB5_LIBRARY)
  message(FATAL_ERROR "Cannot find library krb5, try to install libkrb5-dev.")
endif()

if (CURL_CARES_ENABLED)
find_library(LIBCARES yandex-taxi-cares)
if(NOT LIBCARES)
  message(FATAL_ERROR "Cannot find library yandex-taxi-cares, try to install libyandex-taxi-c-ares-dev.")
endif()
endif (CURL_CARES_ENABLED)

find_package(OpenSSL REQUIRED)
find_package(Crypto REQUIRED)
find_package(Nghttp2 REQUIRED)

set(LIBCURL_YANDEX_LIBRARIES
	${LIBCURL_YANDEX}
	${LIBCARES}
	${LIBGCRYPT}
	${LIBGSSAPI_LIBRARY}
	${LIBKRB5_LIBRARY}
	${Nghttp2_LIBRARIES}
	${OPENSSL_LIBRARIES}
	${Crypto_LIBRARIES})
set(LIBCURL_YANDEX_DEFINITIONS -DCURL_STATICLIB=1)
find_package_handle_standard_args(CURLYANDEX LIBCURL_YANDEX_LIBRARIES LIBCURL_YANDEX_INCLUDE_DIR)
mark_as_advanced(LIBCURL_YANDEX_LIBRARIES LIBCURL_YANDEX_INCLUDE_DIR)

if(NOT LIBCURL_YANDEX_LIBRARIES OR NOT LIBCURL_YANDEX_INCLUDE_DIR)
  message(FATAL_ERROR "Cannot find development files for the libyandex-taxi-curl library. Please try installing libyandex-taxi-curl4-openssl-dev.")
endif()

endif(NOT CURL_YANDEX_ENABLED)

if (LIBCURL_YANDEX_LIBRARIES AND NOT TARGET CurlYandex)
  add_library(CurlYandex INTERFACE)
  target_link_libraries(CurlYandex INTERFACE ${LIBCURL_YANDEX_LIBRARIES})
  target_include_directories(CurlYandex INTERFACE ${LIBCURL_YANDEX_INCLUDE_DIR})
endif()

message(STATUS "Found LibCurl: ${LIBCURL_YANDEX_LIBRARIES}")
