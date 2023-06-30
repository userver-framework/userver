set(USERVER_GOOGLE_COMMON_PROTOS_TARGET "" CACHE STRING "Name of cmake target preparing google common proto library")
set(USERVER_GOOGLE_COMMON_PROTOS "" CACHE PATH "Path to the folder with google common proto files")

if (USERVER_GOOGLE_COMMON_PROTOS_TARGET)
  set(api-common-proto_LIBRARY ${USERVER_GOOGLE_COMMON_PROTOS_TARGET})
  return()
elseif (USERVER_GOOGLE_COMMON_PROTOS)
  set(api-common-protos_SOURCE_DIR ${USERVER_GOOGLE_COMMON_PROTOS})
endif()

if (NOT api-common-protos_SOURCE_DIR)
  set(api-common-protos_SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/api-common-protos)
endif()

if (NOT EXISTS ${api-common-protos_SOURCE_DIR})
  include(FetchContent)

  FetchContent_Declare(
    api-common-protos_external_project
    GIT_REPOSITORY https://github.com/googleapis/api-common-protos.git
    TIMEOUT 10
    GIT_TAG 1.50.0
    SOURCE_DIR ${api-common-protos_SOURCE_DIR}
  )

  FetchContent_GetProperties(api-common-protos_external_project)
  if (NOT api-common-protos_external_project_POPULATED AND
      # POPULATED check glitches with multiple build directories
      NOT EXISTS ${api-common-protos_SOURCE_DIR})
    message(STATUS "Downloading api-common-protos from remote")
    FetchContent_Populate(api-common-protos_external_project)
  endif()
endif()

if (NOT api-common-protos_SOURCE_DIR)
  message(FATAL_ERROR "Unable to get google common proto apis. It is required for userver-grpc build.")
endif()

include(GrpcTargets)
file(GLOB_RECURSE SOURCES
  ${api-common-protos_SOURCE_DIR}/*.proto)

generate_grpc_files(
  PROTOS ${SOURCES}
  INCLUDE_DIRECTORIES ${api-common-protos_SOURCE_DIR}
  SOURCE_PATH ${api-common-protos_SOURCE_DIR}
  GENERATED_INCLUDES include_paths
  CPP_FILES generated_sources
  CPP_USRV_FILES generated_usrv_sources
)

userver_add_library(userver-api-common-protos SOURCES ${generated_sources})

set_target_properties(userver-api-common-protos PROPERTIES LINKER_LANGUAGE CXX)

target_compile_options(userver-api-common-protos PUBLIC -Wno-unused-parameter)
target_include_directories(userver-api-common-protos SYSTEM PUBLIC
  $<BUILD_INTERFACE:${include_paths}>
  $<INSTALL_INTERFACE:include/userver/grpc/proto>)

target_link_libraries(userver-api-common-protos PUBLIC userver-core userver-grpc-deps)

set(api-common-proto_LIBRARY userver-api-common-protos)
set(api-common-proto_USRV_SOURCES ${generated_usrv_sources})

# workaround for INSTALL dependent libraries without install this static library
set_target_properties(userver-api-common-protos PROPERTIES LINKER_LANGUAGE CXX)

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/proto"
  DESTINATION "include/userver/grpc/"
  FILES_MATCHING
  PATTERN "*.pb.hpp"
  PATTERN "*.pb.h"
  )

userver_export(TARGETS userver-api-common-protos)
