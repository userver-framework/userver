include_guard(GLOBAL)

if(userver_grpc_FOUND)
  return()
endif()

set(USERVER_GRPC_SCRIPTS_PATH "${USERVER_CMAKE_DIR}/grpc")


set(USERVER_DOWNLOAD_PACKAGES OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${USERVER_CMAKE_DIR}/GrpcTargets.cmake")

include_directories(${USERVER_CMAKE_DIR}/proto_generated/proto)

add_library(userver::grpc ALIAS userver::userver-grpc-internal)

set(userver_grpc_FOUND TRUE)
