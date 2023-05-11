set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

find_package(gRPC QUIET)
if(gRPC_FOUND)
  return()
endif()

if(USERVER_FEATURE_DOWNLOAD_GRPC)
  message(FATAL_ERROR "Unsupported")
else()
  # Find the system gRPC package
  set(GRPC_USE_SYSTEM_PACKAGE ON CACHE INTERNAL)

  find_package(UserverGrpc REQUIRED)
  set(gRPC_VERSION "${UserverGrpc_VERSION}" CACHE INTERNAL)

  if(NOT TARGET gRPC::grpc++)
    add_library(gRPC::grpc++ ALIAS UserverGrpc)
  endif()

  if(NOT TARGET gRPC::grpc_cpp_plugin)
    find_program(PROTO_GRPC_CPP_PLUGIN_RAW grpc_cpp_plugin)
    add_executable(gRPC::grpc_cpp_plugin IMPORTED)
    set_target_properties(gRPC::grpc_cpp_plugin PROPERTIES IMPORTED_LOCATION "${PROTO_GRPC_CPP_PLUGIN_RAW}")
  endif()
endif()
