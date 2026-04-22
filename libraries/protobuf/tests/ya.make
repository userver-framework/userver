GTEST()

ALLOCATOR(J)

SUBSCRIBER(
    g:taxi-common
)

PEERDIR(
    taxi/uservices/userver/libraries/protobuf
)

USRV_ALL_SRCS()

ADDINCL(
    taxi/uservices/userver/libraries/protobuf/tests
)

END()

RECURSE(
    json
    proto
)
