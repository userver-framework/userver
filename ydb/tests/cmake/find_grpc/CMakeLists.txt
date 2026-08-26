cmake_minimum_required(VERSION 3.14)
project(userver-find-ydb-grpc-test NONE)

if(NOT USERVER_YDB_CMAKE_MODULE_DIR)
    message(FATAL_ERROR "USERVER_YDB_CMAKE_MODULE_DIR is required")
endif()

list(PREPEND CMAKE_MODULE_PATH "${USERVER_YDB_CMAKE_MODULE_DIR}")

add_library(gRPC::grpc++ INTERFACE IMPORTED GLOBAL)
set_property(GLOBAL PROPERTY userver_grpc_cpp_plugin "${CMAKE_COMMAND}")
set(_userver_ydb_grpc_version "1.59.1")

find_package(gRPC 1.41 REQUIRED)

if(NOT TARGET gRPC::grpc_cpp_plugin)
    message(FATAL_ERROR "FindgRPC did not create gRPC::grpc_cpp_plugin")
endif()
if(NOT "${gRPC_VERSION}" STREQUAL "1.59.1")
    message(FATAL_ERROR "FindgRPC returned unexpected version: ${gRPC_VERSION}")
endif()

get_target_property(grpc_cpp_plugin gRPC::grpc_cpp_plugin IMPORTED_LOCATION)
if(NOT "${grpc_cpp_plugin}" STREQUAL "${CMAKE_COMMAND}")
    message(FATAL_ERROR "FindgRPC returned unexpected grpc_cpp_plugin: ${grpc_cpp_plugin}")
endif()
