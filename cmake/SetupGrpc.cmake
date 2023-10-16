cmake_policy(SET CMP0079 NEW)

option(USERVER_DOWNLOAD_PACKAGE_GRPC "Download and setup gRPC"
    ${USERVER_DOWNLOAD_PACKAGES})

macro(try_find_cmake_grpc)
  find_package(gRPC QUIET)

  if(gRPC_FOUND)
    # Use the found CMake-enabled gRPC package
    get_target_property(PROTO_GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
    get_target_property(PROTO_GRPC_PYTHON_PLUGIN gRPC::grpc_python_plugin LOCATION)
    set(PROTOBUF_PROTOC "${Protobuf_PROTOC_EXECUTABLE}")
  endif()
endmacro()

macro(try_find_system_grpc)
  # Find the system gRPC package
  set(GRPC_USE_SYSTEM_PACKAGE ON CACHE INTERNAL "")

  if(USERVER_DOWNLOAD_PACKAGE_GRPC)
    find_package(UserverGrpc QUIET)
  else()
    find_package(UserverGrpc REQUIRED)
  endif()

  if(UserverGrpc_FOUND)
    set(gRPC_VERSION "${UserverGrpc_VERSION}" CACHE INTERNAL "")

    if(NOT TARGET gRPC::grpc++)
      add_library(gRPC::grpc++ ALIAS UserverGrpc)
    endif()

    find_program(PROTO_GRPC_CPP_PLUGIN grpc_cpp_plugin)
    find_program(PROTO_GRPC_PYTHON_PLUGIN grpc_python_plugin)
    set(PROTOBUF_PROTOC "${Protobuf_PROTOC_EXECUTABLE}")
  endif()
endmacro()

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
  try_find_cmake_grpc()
  if(gRPC_FOUND)
    return()
  endif()

  try_find_system_grpc()
  if(UserverGrpc_FOUND)
    return()
  endif()
endif()

include(SetupAbseil)
include(SetupCAres)
include(SetupProtobuf)
include(DownloadUsingCPM)

CPMAddPackage(
    NAME gRPC
    VERSION 1.55.0
    GITHUB_REPOSITORY grpc/grpc
    SYSTEM
    OPTIONS
    "BUILD_SHARED_LIBS OFF"
    "CARES_BUILD_TOOLS OFF"
    "RE2_BUILD_TESTING OFF"
    "OPENSSL_NO_ASM ON"
    "gRPC_BUILD_TESTS OFF"
    "gRPC_BUILD_GRPC_NODE_PLUGIN OFF"
    "gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF"
    "gRPC_BUILD_GRPC_PHP_PLUGIN OFF"
    "gRPC_BUILD_GRPC_RUBY_PLUGIN OFF"
    "gRPC_ZLIB_PROVIDER package"
    "gRPC_CARES_PROVIDER package"
    # TODO if we ever decide to use re2 ourselves, this will be a conflict
    # TODO should use 'package' and download it using CPM instead
    "gRPC_RE2_PROVIDER module"
    "gRPC_SSL_PROVIDER package"
    "gRPC_PROTOBUF_PROVIDER package"
    "gRPC_BENCHMARK_PROVIDER none"
    "gRPC_ABSL_PROVIDER none"
    "gRPC_CARES_LIBRARIES c-ares::cares"
    "gRPC_INSTALL OFF"
)

set(gRPC_VERSION "${CPM_PACKAGE_gRPC_VERSION}")
set(PROTO_GRPC_CPP_PLUGIN $<TARGET_FILE:grpc_cpp_plugin>)
set(PROTO_GRPC_PYTHON_PLUGIN $<TARGET_FILE:grpc_python_plugin>)
set(PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(USERVER_GENERATE_PROTOS_AT_BUILD_TIME TRUE)
write_package_stub(gRPC)
add_library(gRPC::grpc++ ALIAS grpc++)
add_library(gRPC::grpcpp_channelz ALIAS grpcpp_channelz)
mark_targets_as_system("${gRPC_SOURCE_DIR}")
