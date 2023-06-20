PROGRAM(userver-functional-test-service)

ALLOCATOR(J)

OWNER(g:taxi-common)

SRCDIR(taxi/uservices/userver/grpc/functional_tests/metrics)

PEERDIR(
    contrib/libs/grpc
    contrib/libs/grpc/grpcpp_channelz
    contrib/libs/protobuf
    taxi/uservices/userver/grpc/functional_tests/metrics/proto
    taxi/uservices/userver/core
    taxi/uservices/userver/grpc
)

SRCS(
    service.cpp
)

END()

RECURSE_FOR_TESTS(tests)
