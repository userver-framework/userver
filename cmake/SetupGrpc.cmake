find_package(gRPC QUIET)

if(gRPC_FOUND)
  # Use the found CMake-enabled gRPC package
  get_target_property(PROTO_GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
  get_target_property(PROTO_GRPC_PYTHON_PLUGIN gRPC::grpc_python_plugin LOCATION)
  set(PROTO_GRPC_CPP_PLUGIN "${PROTO_GRPC_CPP_PLUGIN}" CACHE INTERNAL "")
elseif(USERVER_FEATURE_DOWNLOAD_GRPC)
  # Download gRPC library from GitHub and use as a submodule
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
  find_program(PROTO_GRPC_PYTHON_PLUGIN grpc_python_plugin)
endif()
