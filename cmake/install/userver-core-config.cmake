include_guard(GLOBAL)

if(userver_core_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS universal)

find_package(Boost REQUIRED CONFIG COMPONENTS locale iostreams)

find_package(ZLIB REQUIRED)
find_package(c-ares REQUIRED)
find_package(libnghttp2 REQUIRED)
find_package(libev REQUIRED)

if(USERVER_CONAN)
    find_package(concurrentqueue REQUIRED)
    find_package(CURL "7.68" REQUIRED)
else()
    include("${USERVER_CMAKE_DIR}/SetupCURL.cmake")
endif()

include("${USERVER_CMAKE_DIR}/UserverTestsuite.cmake")

set(userver_core_FOUND TRUE)
