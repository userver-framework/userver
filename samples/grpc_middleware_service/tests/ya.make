SUBSCRIBER(g:taxi-common)

PY3TEST()

SIZE(SMALL)

TEST_SRCS(
    conftest.py
    taxi/uservices/userver-arc-utils/functional_tests/basic/conftest.py
    test_middlewares.py
)

PEERDIR(
    contrib/python/grpcio
    contrib/python/yandex-taxi-testsuite
    taxi/uservices/userver/testsuite/pytest_plugins/pytest_userver
    taxi/uservices/userver/testsuite/pytest_plugins/pytest_userver/plugins/grpc
    taxi/uservices/userver/samples/grpc_middleware_service/proto
    contrib/python/psycopg2
    contrib/python/requests
)

DEPENDS(
    taxi/uservices/userver/samples/grpc_middleware_service
)

DATA(
    arcadia/taxi/uservices/userver/samples/grpc_middleware_service
)

CONFTEST_LOAD_POLICY_LOCAL()
TEST_CWD(/)

END()
