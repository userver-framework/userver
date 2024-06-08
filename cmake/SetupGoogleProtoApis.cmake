set(USERVER_GOOGLE_COMMON_PROTOS_TARGET "" CACHE STRING "Name of cmake target preparing google common proto library")
set(USERVER_GOOGLE_COMMON_PROTOS "" CACHE PATH "Path to the folder with google common proto files")

if (USERVER_GOOGLE_COMMON_PROTOS_TARGET)
  set(api-common-proto_LIBRARY ${USERVER_GOOGLE_COMMON_PROTOS_TARGET})
  return()
elseif (USERVER_GOOGLE_COMMON_PROTOS)
  set(api-common-protos_SOURCE_DIR ${USERVER_GOOGLE_COMMON_PROTOS})
endif()

if (NOT EXISTS ${api-common-protos_SOURCE_DIR})
  include(DownloadUsingCPM)
  CPMAddPackage(
      NAME api-common-protos
      VERSION 1.50.0
      GITHUB_REPOSITORY googleapis/api-common-protos
      GIT_TAG 1.50.0
      DOWNLOAD_ONLY YES
  )
endif()

if (NOT api-common-protos_SOURCE_DIR)
  message(FATAL_ERROR "Unable to get google common proto apis. It is required for userver-grpc build.")
endif()

include(GrpcTargets)
file(GLOB_RECURSE SOURCES
  ${api-common-protos_SOURCE_DIR}/*.proto)

userver_generate_grpc_files(
  PROTOS ${SOURCES}
  INCLUDE_DIRECTORIES ${api-common-protos_SOURCE_DIR}
  SOURCE_PATH ${api-common-protos_SOURCE_DIR}
  GENERATED_INCLUDES include_paths
  CPP_FILES generated_sources
  CPP_USRV_FILES generated_usrv_sources
)

add_library(userver-api-common-protos STATIC ${generated_sources})
target_compile_options(userver-api-common-protos PUBLIC -Wno-unused-parameter)
target_include_directories(userver-api-common-protos SYSTEM PUBLIC $<BUILD_INTERFACE:${include_paths}>)
target_link_libraries(userver-api-common-protos PUBLIC userver-grpc-deps)

_userver_directory_install(COMPONENT grpc
  DIRECTORY ${include_paths}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/proto_generated
)

set(api-common-proto_LIBRARY userver-api-common-protos)
set(api-common-proto_USRV_SOURCES ${generated_usrv_sources})
