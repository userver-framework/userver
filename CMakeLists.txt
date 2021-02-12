cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0025 NEW)
project(userver)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_generated
)

include(UserverCodegen)
include(SetupEnvironment)
include(AddGoogleTests)
include(CheckSubmodule)
include(Sanitizers)
include(FindPackageRequired)

set(USERVER_THIRD_PARTY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party CACHE INTERNAL "")

init_debian_depends()

add_subdirectory(core "${CMAKE_BINARY_DIR}/userver/core")
add_subdirectory("${USERVER_THIRD_PARTY_DIRS}/boost_stacktrace")
add_subdirectory(uboost_coro)

if(NOT USERVICES)
  add_subdirectory(testsuite)
  add_subdirectory(tools/engine)
  add_subdirectory(tools/json2yaml)
  add_subdirectory(tools/httpclient)
  add_subdirectory(tools/netcat)
  add_subdirectory(tools/congestion_control_emulator)
  add_subdirectory(samples)

  add_subdirectory(mongo "${CMAKE_BINARY_DIR}/userver/mongo")
  add_subdirectory(postgresql "${CMAKE_BINARY_DIR}/userver/postgresql")
  add_subdirectory(redis "${CMAKE_BINARY_DIR}/userver/redis")
  add_subdirectory(grpc "${CMAKE_BINARY_DIR}/userver/grpc")
  add_subdirectory(universal "${CMAKE_BINARY_DIR}/userver/universal")
endif(NOT USERVICES)
