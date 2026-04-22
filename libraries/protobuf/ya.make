LIBRARY()

SUBSCRIBER(
    g:taxi-common
)

PEERDIR(
    contrib/libs/fmt
    contrib/libs/protobuf
    taxi/uservices/userver/universal
)

INCLUDE(${ARCADIA_ROOT}/taxi/uservices/userver/libraries/protobuf/ya.make.ext)

ADDINCL(
    GLOBAL taxi/uservices/userver/libraries/protobuf/include
    taxi/uservices/userver/libraries/protobuf/src
)

USRV_ALL_SRCS(
    RECURSIVE
    src/
    EXCLUDE
    **/*_benchmark.cpp
    **/*_test.cpp
)

SET(TIDY_HEADER_FILTER "${ARCADIA_ROOT}/taxi/uservices/userver/(universal)/")

END()

RECURSE_FOR_TESTS(
    tests
)
