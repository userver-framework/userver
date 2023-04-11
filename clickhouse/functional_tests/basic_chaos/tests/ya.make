OWNER(g:taxi-common)

PY3TEST()

SIZE(MEDIUM)

INCLUDE(
    ${ARCADIA_ROOT}/taxi/external/recipes/clickhouse/recipe.inc
)

ALL_PYTEST_SRCS()

TEST_SRCS(
    taxi/uservices/userver-arc-utils/functional_tests/basic/conftest.py
)

PEERDIR(
    contrib/python/yandex-taxi-testsuite
    contrib/python/clickhouse-driver/py3
    taxi/uservices/userver/testsuite/pytest_plugins/pytest_userver
)

DEPENDS(
    taxi/uservices/userver/clickhouse/functional_tests/basic_chaos
)

DATA(
    arcadia/taxi/uservices/userver/clickhouse/functional_tests/basic_chaos
)

CONFTEST_LOAD_POLICY_LOCAL()
TEST_CWD(/)

END()
