PROTO_LIBRARY(userver-functional-test-service-grpc-proto)

OWNER(g:taxi-common)

EXCLUDE_TAGS(GO_PROTO JAVA_PROTO)

SRCDIR(taxi/uservices/userver/grpc/functional_tests/metrics/proto)

SRCS(
    samples/greeter.proto
)

PROTO_NAMESPACE(taxi/uservices/userver/grpc/functional_tests/metrics/proto)
PY_NAMESPACE(.)

GRPC()

CPP_PROTO_PLUGIN2(
    usrv_service
    taxi/uservices/userver-arc-utils/scripts/grpc/arc-generate-service
    _service.usrv.pb.hpp
    _service.usrv.pb.cpp
    DEPS taxi/uservices/userver/grpc
)
CPP_PROTO_PLUGIN2(
    usrv_client
    taxi/uservices/userver-arc-utils/scripts/grpc/arc-generate-client
    _client.usrv.pb.hpp
    _client.usrv.pb.cpp
    DEPS taxi/uservices/userver/grpc
)

END()


