cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0025 NEW)
project(userver)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(IgnoreVars)
include(SetupEnvironment)
include(AddGoogleTests)
include(CheckSubmodule)
include(CompileFbSchema)
include(Sanitizers)
include(FindPackageRequired)

set(USERVER_THIRD_PARTY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party CACHE INTERNAL "")

init_debian_depends()
append_debian_depends("clang-7 lld-7")

check_submodule(${CMAKE_CURRENT_SOURCE_DIR}/backend-common-cpp CMakeLists.txt)
add_subdirectory(backend-common-cpp)

find_package(ExternalProjectSpdlog)
target_include_directories(
  yandex-taxi-backend-common-cpp
  PUBLIC
    core/include/
    # TODO: flatbuffers should have public include in their CMakeLists.txt
    backend-common-cpp/third_party/flatbuffers/include/
  PRIVATE
    core/src/
    ${SPDLOG_INCLUDE_DIRS}
)
add_dependencies(yandex-taxi-backend-common-cpp spdlog_lib)

add_subdirectory(core)

set(YANDEX_TAXI_USERVER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")

if(NOT USERVICES)
  add_subdirectory(testsuite)
  add_subdirectory(tools/engine)
  add_subdirectory(tools/httpclient)
  add_subdirectory(tools/netcat)

  find_package(Helperyandex-taxi-userver-core)
  find_package(Helperyandex-taxi-userver-mongo)
  find_package(Helperyandex-taxi-userver-postgresql)
  find_package(Helperyandex-taxi-userver-redis)
endif(NOT USERVICES)
