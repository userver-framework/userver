OWNER(g:taxi-common)

PY3TEST()

SIZE(MEDIUM)

ALL_PYTEST_SRCS(RECURSIVE)

TEST_SRCS(
    taxi/uservices/userver-arc-utils/functional_tests/basic/conftest.py
)

PEERDIR(
    contrib/python/yandex-taxi-testsuite
    taxi/uservices/userver/testsuite/pytest_plugins/pytest_userver
)

DEPENDS(
    taxi/uservices/userver/core/functional_tests/cache_update
)

DATA(
    arcadia/taxi/uservices/userver/core/functional_tests/cache_update
)

CONFTEST_LOAD_POLICY_LOCAL()
TEST_CWD(/)

END()
