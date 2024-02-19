include_guard(GLOBAL)

if(userver_universal_FOUND)
  return()
endif()

find_package(Threads)
find_package(Boost REQUIRED COMPONENTS
    program_options
    filesystem
    regex
    stacktrace_basic
    stacktrace_backtrace
)
find_package(Iconv REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(fmt "8.1.1" REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
find_package(cctz REQUIRED)
find_package(CryptoPP REQUIRED)
find_package(libyamlcpp REQUIRED)

include("${USERVER_CMAKE_DIR}/AddGoogleTests.cmake")
include("${USERVER_CMAKE_DIR}/Sanitizers.cmake")
include("${USERVER_CMAKE_DIR}/SetupLTO.cmake")
include("${USERVER_CMAKE_DIR}/SetupLinker.cmake")

_userver_make_sanitize_blacklist()

add_library(userver::universal ALIAS userver::userver-universal)

set(userver_universal_FOUND TRUE)
