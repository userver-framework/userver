OWNER(g:taxi-common)

PY3TEST()

SIZE(MEDIUM)

ALL_PYTEST_SRCS()

TEST_SRCS(
    taxi/uservices/userver-arc-utils/functional_tests/basic/conftest.py
)

PEERDIR(
    contrib/python/grpcio
    contrib/python/yandex-taxi-testsuite
    taxi/uservices/userver/testsuite/pytest_plugins/pytest_userver/plugins/grpc
    taxi/uservices/userver/grpc/functional_tests/basic_chaos/proto
)

DEPENDS(
    taxi/uservices/userver/grpc/functional_tests/metrics
)

DATA(
    arcadia/taxi/uservices/userver/grpc/functional_tests/metrics
)

CONFTEST_LOAD_POLICY_LOCAL()
TEST_CWD(/)

END()
