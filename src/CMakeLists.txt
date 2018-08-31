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

file(GLOB_RECURSE TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)
list (REMOVE_ITEM SOURCES ${TEST_SOURCES})


set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost REQUIRED COMPONENTS coroutine)
find_package(Jemalloc REQUIRED)
find_package(Hiredis REQUIRED)
find_package(HttpParser REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(LibEv REQUIRED)
find_package(MongoClient REQUIRED)
find_package(ZLIB REQUIRED)
find_package(ExternalProjectGTest)
find_package(CurlYandex REQUIRED)

add_library(${PROJECT} STATIC ${SOURCES})
set_target_properties(${PROJECT} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT}
  PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    ${JEMALLOC_LIBRARIES}
    yandex-taxi-backend-common-cpp
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


message("gtest: ${GTEST_LIBRARIES}")
add_executable(${PROJECT}_unittest ${TEST_SOURCES})
target_link_libraries(${PROJECT}_unittest
  ${PROJECT}
  ${GTEST_LIBRARIES}
  ${GMOCK_LIBRARIES}
)
target_include_directories(${PROJECT}_unittest PRIVATE
  $<TARGET_PROPERTY:${PROJECT},INCLUDE_DIRECTORIES>
)

add_google_tests(${PROJECT}_unittest)
