cmake_minimum_required(VERSION 3.5)

set(PROJECT yandex-taxi-userver)
project(${PROJECT} VERSION ${DEBVERSION})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/build_config.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/build_config.hpp
  ESCAPE_QUOTES @ONLY)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

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

add_library(${PROJECT} STATIC ${SOURCES})
set_target_properties(${PROJECT} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT}
  PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    ${JEMALLOC_LIBRARIES}
  PRIVATE
    ${Boost_COROUTINE_LIBRARY}
    ${JSONCPP_LIBRARIES}
    ${HIREDIS_LIBRARIES}
    ${HTTP_PARSER_LIBRARIES}
    ${LIBEV_LIBRARIES}
    ${MONGOCLIENT_LIBRARIES}
)

target_include_directories(${PROJECT} PUBLIC ${USERVER_PUBLIC_INCLUDE_DIR})
# FIXME: change to PRIVATE once we have interface
target_include_directories(${PROJECT} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${USERVER_THIRD_PARTY_DIRS}
  ${JSONCPP_INCLUDE_DIR}
  ${LIBEV_INCLUDE_DIR}
  ${MONGOCLIENT_INCLUDE_DIR}
)
