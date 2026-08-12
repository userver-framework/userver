SUBSCRIBER(g:taxi-common)

PY3TEST()

SIZE(SMALL)

ALL_PYTEST_SRCS()

PEERDIR(
    taxi/uservices/userver/testsuite/pytest_plugins/pytest_userver
    taxi/uservices/userver-arc-utils/functional_tests/pytest_plugins
)

DEPENDS(
    taxi/uservices/userver/samples/chaotic_openapi_service
)

DATA(
    arcadia/taxi/uservices/userver/samples/chaotic_openapi_service
)

CONFTEST_LOAD_POLICY_LOCAL()
TEST_CWD(/)

END()
