include(UserverGrpcTargets)
userver_add_grpc_library(${PROJECT_NAME}-unused-proto PROTOS samples/unused.proto)
