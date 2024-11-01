LIBRARY()

SUBSCRIBER(
    g:taxi-common
)

PEERDIR(
    contrib/libs/pugixml
    taxi/uservices/userver/core
)


INCLUDE(${ARCADIA_ROOT}/taxi/uservices/userver/libraries/s3api/ya.make.ext)

ADDINCL(
    GLOBAL taxi/uservices/userver/libraries/s3api/include
    contrib/libs/pugixml
    taxi/uservices/userver/libraries/s3api/src
)

USRV_ALL_SRCS(
    RECURSIVE
    src/
    EXCLUDE
    **/*_benchmark.cpp
    **/*_test.cpp
)

END()

RECURSE_FOR_TESTS(
    tests
    utest
)
