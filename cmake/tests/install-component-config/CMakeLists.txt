cmake_minimum_required(VERSION 3.14)
project(userver-install-component-config-test NONE)

if(NOT USERVER_SOURCE_DIR)
    message(FATAL_ERROR "USERVER_SOURCE_DIR is required")
endif()

set(USERVER_INSTALL ON)
set(USERVER_ROOT_DIR "${USERVER_SOURCE_DIR}")

set(CMAKE_INSTALL_LIBDIR lib)
include("${USERVER_ROOT_DIR}/cmake/ModuleHelpers.cmake")
include("${USERVER_ROOT_DIR}/cmake/install-support/InstallComponentConfigGen.cmake")

add_library(userver-easy INTERFACE)
set_property(GLOBAL PROPERTY USERVER_EXPORTED_COMPONENTS easy)
set_property(GLOBAL PROPERTY USERVER_COMPONENT_TARGETS_easy userver-easy)

_userver_install_component_config(easy)
_userver_generate_and_install_configs()

set(USERVER_CMAKE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
include("${CMAKE_CURRENT_BINARY_DIR}/generated-configs/userver-easy-config.cmake")
if(NOT userver_easy_FOUND)
    message(FATAL_ERROR "Generated component config did not mark userver_easy as found")
endif()
