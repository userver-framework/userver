cmake_minimum_required(VERSION 3.5)

set(PROJECT yandex-taxi-userver)
project(${PROJECT} VERSION ${PROJECT_VERSION_STRING})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/build_config.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/build_config.hpp
  ESCAPE_QUOTES @ONLY)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../backend-common-cpp/*_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)

file(GLOB_RECURSE LIBUTEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/testing/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/testing/*.hpp)

list (REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES} ${LIBUTEST_SOURCES})


set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost REQUIRED COMPONENTS coroutine program_options)
find_package(Jemalloc REQUIRED)
find_package(Hiredis REQUIRED)
find_package(HttpParser REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(LibEv REQUIRED)
find_package(MongoClient REQUIRED)
find_package(ZLIB REQUIRED)
find_package(ExternalProjectGTest)
find_package(ExternalProjectGBench)
find_package(ExternalProjectOpenTracing)
find_package(CurlYandex REQUIRED)

add_library(${PROJECT} STATIC ${SOURCES})
set_target_properties(${PROJECT} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT}
  PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    ${JEMALLOC_LIBRARIES}
    yandex-taxi-backend-common-cpp
    libopentracing
  PRIVATE
    ${LIBCURL_YANDEX_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${Boost_COROUTINE_LIBRARY}
    ${JSONCPP_LIBRARIES}
    ${HIREDIS_LIBRARIES}
    ${HTTP_PARSER_LIBRARIES}
    ${LIBEV_LIBRARIES}
    ${MONGOCLIENT_LIBRARIES}
    ${CLANG_LIBRARIES}
)

set(USERVER_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(USERVER_THIRD_PARTY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

target_include_directories(${PROJECT} PUBLIC
  ${USERVER_PUBLIC_INCLUDE_DIR}
  ${USERVER_THIRD_PARTY_DIRS} # FIXME move to private in TAXICOMMON-32
  ${JSONCPP_INCLUDE_DIR} # FIXME move to private in TAXICOMMON-15
)
# FIXME: change to PRIVATE once we have interface
target_include_directories(${PROJECT} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LIBCURL_YANDEX_INCLUDE_DIR}
  ${LIBEV_INCLUDE_DIR}
  ${MONGOCLIENT_INCLUDE_DIR}
)



add_library(utest STATIC ${LIBUTEST_SOURCES})
set_target_properties(utest PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(utest
  PRIVATE
    Boost::program_options
  PUBLIC
    -Wl,--start-group
        ${PROJECT}
        yandex-taxi-backend-common-cpp
    -Wl,--end-group
    libgtest
    libgmock
)

target_include_directories(utest PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/testing
)



message("gtest: ${GTEST_LIBRARIES}")
add_executable(${PROJECT}_unittest ${UNIT_TEST_SOURCES})
target_link_libraries(${PROJECT}_unittest
  atomic
  utest
)

add_google_tests(${PROJECT}_unittest)
