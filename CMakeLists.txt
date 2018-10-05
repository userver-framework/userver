cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(SetupEnvironment)
include(AddGoogleTests)
include(CppCheck)
include(CheckSubmodule)

set(USERVER_THIRD_PARTY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

check_submodule(${CMAKE_CURRENT_SOURCE_DIR}/backend-common-cpp CMakeLists.txt)
add_subdirectory(backend-common-cpp)

# For logger
find_package(ExternalProjectSpdlog)
target_include_directories(
  yandex-taxi-backend-common-cpp
  PUBLIC
    include/
  PRIVATE
    src/
    ${SPDLOG_INCLUDE_DIRS}
)
add_dependencies(yandex-taxi-backend-common-cpp spdlog_lib)

add_subdirectory(src)
add_subdirectory(tools/engine)
add_subdirectory(tools/httpclient)
add_subdirectory(tools/redisclient)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Enable testing")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Enable parse tools")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Enable contrib stuff in library")
add_subdirectory(${USERVER_THIRD_PARTY_DIRS}/yaml-cpp)
