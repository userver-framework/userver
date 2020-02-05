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

check_submodule(${CMAKE_CURRENT_SOURCE_DIR}/backend-common-cpp CMakeLists.txt)
add_subdirectory(backend-common-cpp)

find_package(Helperspdlog)
target_include_directories(
  yandex-taxi-backend-common-cpp
  PUBLIC
    shared/include/
    core/include/
)
if(NOT FLATBUFFERS_EXTERNAL_ENABLED)
  target_include_directories(
    yandex-taxi-backend-common-cpp
    PUBLIC
      # TODO: flatbuffers should have public include in their CMakeLists.txt
      backend-common-cpp/third_party/flatbuffers/include/
  )
endif()
add_dependencies(yandex-taxi-backend-common-cpp spdlog)

add_subdirectory(core "${CMAKE_BINARY_DIR}/userver/core")
add_subdirectory(uboost_coro)

if(NOT USERVICES)
  add_subdirectory(testsuite)
  add_subdirectory(tools/engine)
  add_subdirectory(tools/json2yaml)
  add_subdirectory(tools/httpclient)
  add_subdirectory(tools/netcat)

  add_subdirectory(mongo "${CMAKE_BINARY_DIR}/userver/mongo")
  add_subdirectory(postgresql "${CMAKE_BINARY_DIR}/userver/postgresql")
  add_subdirectory(redis "${CMAKE_BINARY_DIR}/userver/redis")
  add_subdirectory(grpc "${CMAKE_BINARY_DIR}/userver/grpc")
  add_subdirectory(universal "${CMAKE_BINARY_DIR}/userver/universal")
endif(NOT USERVICES)
