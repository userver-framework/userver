include_guard(GLOBAL)

if(userver_core_FOUND)
  return()
endif()

find_package(Boost REQUIRED COMPONENTS
    locale
    iostreams
)

find_package(CURL "7.68" REQUIRED)
find_package(ZLIB REQUIRED) 

find_package(c-ares 1.16 REQUIRED)
if (c-ares_FOUND)
  if(NOT TARGET c-ares)
    add_library(c-ares ALIAS c-ares::cares)
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
find_package(Http_Parser REQUIRED)
find_package(Nghttp2 REQUIRED)
find_package(LibEv REQUIRED)
find_package(UserverGTest REQUIRED)
find_package(UserverGBench REQUIRED)

include("${USERVER_CMAKE_DIR}/FindPython.cmake")
include("${USERVER_CMAKE_DIR}/UserverTestsuite.cmake")

add_library(userver::core ALIAS userver::userver-core)
add_library(userver::utest ALIAS userver::userver-utest)
add_library(userver::ubench ALIAS userver::userver-ubench)

set(userver_core_FOUND TRUE)
