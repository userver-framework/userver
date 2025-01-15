_userver_module_begin(
    NAME UserverGrpc
    DEBIAN_NAMES libgrpc-dev libgrpc++-dev libgrpc++1 protobuf-compiler-grpc
    FORMULA_NAMES grpc
    PACMAN_NAMES grpc
    PKG_CONFIG_NAMES grpc++
)

_userver_module_find_include(
    NAMES grpc/grpc.h
)

_userver_module_find_library(
    NAMES libgrpc.so
)

_userver_module_find_library(
    NAMES libgrpc++.so
)

_userver_module_find_library(
    NAMES libgpr.so
)

_userver_module_find_library(
    NAMES
    absl
    libabsl_synchronization.so
    grpc  # fallback, old versions of gRPC do not link with absl
)

_userver_module_end()
