cmake_minimum_required(VERSION 3.5)

set(PROJECT yandex-taxi-userver)
project(${PROJECT} VERSION 0.0.0) # FIXME

file(GLOB_RECURSE SOURCES *.cpp *.hpp)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost REQUIRED COMPONENTS coroutine)
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
  .
  ${USERVER_THIRD_PARTY_DIRS}
  ${JSONCPP_INCLUDE_DIR}
  ${LIBEV_INCLUDE_DIR}
  ${MONGOCLIENT_INCLUDE_DIR}
)
