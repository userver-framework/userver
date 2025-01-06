_userver_module_begin(
    NAME GrpcChannelz
    DEBIAN_NAMES libgrpc-dev libgrpc++-dev libgrpc++1 protobuf-compiler-grpc
    FORMULA_NAMES grpc
    PACMAN_NAMES grpc
    PKG_CONFIG_NAMES grpc++
)

_userver_module_find_include(
    NAMES grpc/grpc.h
)

_userver_module_find_library(
    NAMES grpcpp_channelz
)

_userver_module_end()
