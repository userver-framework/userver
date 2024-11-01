LIBRARY()

SUBSCRIBER(
    g:taxi-common
)

PEERDIR(
    taxi/uservices/userver-arc-utils/testing
    taxi/uservices/userver/core
    taxi/uservices/userver/core/utest
    taxi/uservices/userver/libraries/s3api
)

ADDINCL(
    GLOBAL taxi/uservices/userver/libraries/s3api/utest/include
)

USRV_ALL_SRCS(
    RECURSIVE src
)

SET(TIDY_HEADER_FILTER "${ARCADIA_ROOT}/taxi/uservices/userver/(core|s3api|universal)/")

END()

