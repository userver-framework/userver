GTEST()

ALLOCATOR(J)

SUBSCRIBER(
    g:taxi-common
)

PEERDIR(
    taxi/uservices/userver/libraries/protobuf
    taxi/uservices/userver/libraries/protobuf/tests/proto/proto_json
    taxi/uservices/userver/universal/utest
)

USRV_ALL_SRCS()

ADDINCL(
    taxi/uservices/userver/libraries/protobuf/tests
)

END()
