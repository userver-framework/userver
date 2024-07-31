G_BENCHMARK(userver-grpc-benchmark)

ALLOCATOR(J)
SIZE(MEDIUM)

SUBSCRIBER(g:taxi-common)

PEERDIR(
    taxi/uservices/userver/grpc
    taxi/uservices/userver/grpc/proto/tests
)

USRV_ALL_SRCS()

END()
