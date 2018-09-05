cmake_minimum_required(VERSION 3.5)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(SetupEnvironment)
include(AddGoogleTests)
include(GetVersion)
include(CppCheck)
include(CheckSubmodule)


check_submodule(${CMAKE_CURRENT_SOURCE_DIR}/backend-common-cpp CMakeLists.txt)
add_subdirectory(backend-common-cpp)
# For logger
target_include_directories(
    yandex-taxi-backend-common-cpp
    PRIVATE
    src/
    PUBLIC
    third_party/
    include/
)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(tools/httpclient)
add_subdirectory(tools/engine)


option(BUILD_USERVER_AS_SUBMODULE "Disables build of examples and testsuite" OFF) # TODO: remove
if (NOT BUILD_USERVER_AS_SUBMODULE)
    include(TestsuiteSupport)
    add_subdirectory(testsuite)
    add_subdirectory(examples)
endif()
