if(APPLE)
  # Apple has no yandex-taxi's curl, fall back to libcurl
  set(CURL_YANDEX_ENABLED_DEFAULT OFF)
  set(CURL_CARES_ENABLED_DEFAULT OFF)
  # preferably the brewed (newer) one
  set(CMAKE_FIND_FRAMEWORK LAST)
  set(CMAKE_FIND_APPBUNDLE LAST)
  set(CURL_ROOT "/usr/local/opt/curl")
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
set(LIBCURL_YANDEX_VERSION_STRING ${CURL_VERSION_STRING})
set(LIBCURL_YANDEX_DEFINITIONS "")

else(NOT CURL_YANDEX_ENABLED)

find_path(
  LIBCURL_YANDEX_INCLUDE_DIR curl/curl.h
  NO_DEFAULT_PATH
  PATHS
    "/usr/include/libyandex-taxi-curl/")
find_library(LIBCURL_YANDEX yandex-taxi-curl)

# From FindCURL.cmake
if(LIBCURL_YANDEX_INCLUDE_DIR)
  file(STRINGS "${LIBCURL_YANDEX_INCLUDE_DIR}/curl/curlver.h"
    curl_version_str
    REGEX "^#define[\t ]+LIBCURL_VERSION[\t ]+\".*\""
  )
  string(REGEX REPLACE "^#define[\t ]+LIBCURL_VERSION[\t ]+\"([^\"]*)\".*" "\\1"
    LIBCURL_YANDEX_VERSION_STRING
    "${curl_version_str}"
  )
  unset(curl_version_str)
endif(LIBCURL_YANDEX_INCLUDE_DIR)

if (CURL_CARES_ENABLED)
find_library(LIBCARES yandex-taxi-cares)
if(NOT LIBCARES)
  message(FATAL_ERROR "Cannot find library yandex-taxi-cares, try to install libyandex-taxi-c-ares-dev.")
endif()
endif (CURL_CARES_ENABLED)

find_package(Brotli REQUIRED)
find_package(Nghttp2 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Crypto REQUIRED)
find_package(ZLIB REQUIRED)

set(LIBCURL_YANDEX_LIBRARIES
  ${LIBCURL_YANDEX}
  ${LIBCARES}
  ${Brotli_LIBRARIES}
  ${Nghttp2_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${Crypto_LIBRARIES}
  ${ZLIB_LIBRARIES}
)
set(LIBCURL_YANDEX_DEFINITIONS -DCURL_STATICLIB=1)

if(NOT LIBCURL_YANDEX_LIBRARIES OR NOT LIBCURL_YANDEX_INCLUDE_DIR)
  message(FATAL_ERROR "Cannot find development files for the libyandex-taxi-curl library. Please try installing libyandex-taxi-curl4-openssl-dev.")
endif()

endif(NOT CURL_YANDEX_ENABLED)

find_package_handle_standard_args(CurlYandex
  REQUIRED_VARS LIBCURL_YANDEX_LIBRARIES LIBCURL_YANDEX_INCLUDE_DIR
  VERSION_VAR LIBCURL_YANDEX_VERSION_STRING
)
mark_as_advanced(
  LIBCURL_YANDEX_LIBRARIES
  LIBCURL_YANDEX_INCLUDE_DIR
  LIBCURL_YANDEX_VERSION_STRING
)

if (LIBCURL_YANDEX_LIBRARIES AND NOT TARGET CurlYandex)
  add_library(CurlYandex INTERFACE)
  target_link_libraries(CurlYandex INTERFACE ${LIBCURL_YANDEX_LIBRARIES})
  target_include_directories(CurlYandex INTERFACE ${LIBCURL_YANDEX_INCLUDE_DIR})
endif()
