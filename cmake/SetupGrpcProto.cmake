set(USERVER_GRPC_PROTO_TARGET "" CACHE STRING "Name of cmake target preparing grpc proto library")
set(USERVER_GRPC_PROTO "" CACHE PATH "Path to the folder with grpc proto files")

if (USERVER_GRPC_PROTO_TARGET)
  set(grpc-proto_LIBRARY ${USERVER_GRPC_PROTO_TARGET})
  return()
elseif (USERVER_GRPC_PROTO)
  set(grpc-proto_SOURCE_DIR ${USERVER_GRPC_PROTO})
endif()

if (NOT EXISTS ${grpc-proto_SOURCE_DIR})
  include(DownloadUsingCPM)
  CPMAddPackage(
    NAME grpc-proto
    GIT_TAG master
    GITHUB_REPOSITORY grpc/grpc-proto
    DOWNLOAD_ONLY YES
  )
endif()

if(NOT TARGET userver-api-common-protos)
  include(SetupGoogleProtoApis)
endif()

if (NOT api-common-protos_SOURCE_DIR)
  message(FATAL_ERROR "Unable to get google common proto apis. It is required for userver-grpc build.")
endif()

if (NOT grpc-proto_SOURCE_DIR)
  message(FATAL_ERROR "Unable to get grpc-proto. It is required for userver-grpc build.")
endif()

SET(SOURCES
  ${grpc-proto_SOURCE_DIR}/grpc/lookup/v1/rls_config.proto
  ${grpc-proto_SOURCE_DIR}/grpc/service_config/service_config.proto
)

include(UserverGrpcTargets)
userver_generate_grpc_files(
  PROTOS ${SOURCES}
  INCLUDE_DIRECTORIES ${grpc-proto_SOURCE_DIR} ${api-common-protos_SOURCE_DIR}
  SOURCE_PATH ${grpc-proto_SOURCE_DIR}
  GENERATED_INCLUDES include_paths
  CPP_FILES generated_sources
  CPP_USRV_FILES generated_usrv_sources
)

add_library(userver-grpc-proto-contrib STATIC ${generated_sources})
target_link_libraries(userver-grpc-proto-contrib PUBLIC ${api-common-proto_LIBRARY})
target_compile_options(userver-grpc-proto-contrib PRIVATE -Wno-deprecated-declarations)
target_include_directories(userver-grpc-proto-contrib SYSTEM PUBLIC $<BUILD_INTERFACE:${include_paths}>)

_userver_directory_install(COMPONENT grpc
  DIRECTORY ${include_paths}/grpc
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/userver/third_party
  PATTERN "*.pb.h"
)

set(grpc-proto_LIBRARY userver-grpc-proto-contrib)
