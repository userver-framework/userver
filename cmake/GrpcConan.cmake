
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)
set(GRPC_LIBRARY_VERSION "${gRPC_VERSION}")
set(GRPC_PROTOBUF_INCLUDE_DIRS "${protobuf_INCLUDE_DIR}" CACHE PATH INTERNAL)
