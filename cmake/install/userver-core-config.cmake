include_guard(GLOBAL)

if(userver_core_FOUND)
  return()
endif()

find_package(userver REQUIRED COMPONENTS
    universal
)

find_package(Boost REQUIRED CONFIG COMPONENTS
    locale
    iostreams
)

find_package(ZLIB REQUIRED)

include("${USERVER_CMAKE_DIR}/UserverTestsuite.cmake")

if (USERVER_CONAN)
  find_package(c-ares REQUIRED CONFIG)
  find_package(libnghttp2 REQUIRED CONFIG)
  find_package(libev REQUIRED CONFIG)
  find_package(concurrentqueue REQUIRED CONFIG)
  find_package(CURL "7.68" REQUIRED)
else()
  find_package(Nghttp2 REQUIRED)
  find_package(LibEv REQUIRED)
  find_package(c-ares REQUIRED)
  if (c-ares_FOUND AND NOT TARGET c-ares::cares)
    add_library(c-ares::cares ALIAS c-ares)
  endif()
  include("${USERVER_CMAKE_DIR}/SetupCURL.cmake")
endif()

set(userver_core_FOUND TRUE)
