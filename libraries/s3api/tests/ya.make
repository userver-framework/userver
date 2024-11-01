GTEST()

SIZE(MEDIUM)

TAG()

CFLAGS(
    -DCURRENT_SOURCE_ROOT='"taxi/uservices/userver/libraries/s3api"'
)

PEERDIR(
    library/cpp/testing/common
    taxi/uservices/userver/libraries/s3api
    taxi/uservices/userver-arc-utils/testing
    taxi/uservices/userver/core/testing
)

ADDINCL(
    ${ARCADIA_BUILD_ROOT}/taxi/uservices/userver/libraries/s3api/taxi/uservices/userver/libraries/s3api/src
    taxi/uservices/userver/libraries/s3api/src
)

DATA_FILES(
    taxi/uservices/userver/libraries/s3api
)

USRV_ALL_SRCS(
    RECURSIVE
    ../src
    SUFFIX
    _test
)

END()
