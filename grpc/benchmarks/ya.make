G_BENCHMARK(userver-grpc-benchmark)

ALLOCATOR(J)
SIZE(MEDIUM)

OWNER(g:taxi-common)

PEERDIR(
    taxi/uservices/userver-arc-utils/grpc/gen/grpc
    taxi/uservices/userver/grpc/utest
    taxi/uservices/userver/grpc
)

USRV_ALL_SRCS()

END()
