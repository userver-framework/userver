if(TARGET gRPC::grpc++)
  return()
endif()

find_package(gRPC QUIET)
if(gRPC_FOUND)
  get_target_property(PROTO_GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
  set(PROTO_GRPC_CPP_PLUGIN "${PROTO_GRPC_CPP_PLUGIN}" CACHE INTERNAL "")
  return()
endif()

if(USERVER_FEATURE_DOWNLOAD_GRPC)
  message(FATAL_ERROR "Unsupported")
else()
  # Find the system gRPC package
  set(GRPC_USE_SYSTEM_PACKAGE ON CACHE INTERNAL "")

  find_package(UserverGrpc REQUIRED)
  set(gRPC_VERSION "${UserverGrpc_VERSION}" CACHE INTERNAL "")

  if(NOT TARGET gRPC::grpc++)
    add_library(gRPC::grpc++ ALIAS UserverGrpc)
  endif()

  find_program(PROTO_GRPC_CPP_PLUGIN grpc_cpp_plugin)
endif()
