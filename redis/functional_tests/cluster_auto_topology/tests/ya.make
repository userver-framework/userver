SUBSCRIBER(g:taxi-common)

PY3TEST()

SIZE(MEDIUM)

INCLUDE(
    ${ARCADIA_ROOT}/taxi/external/recipes/redis/recipe.inc
)

ALL_PYTEST_SRCS()

TEST_SRCS(
    taxi/uservices/userver-arc-utils/functional_tests/basic/conftest.py
)

PEERDIR(
    contrib/python/redis
    contrib/python/yandex-taxi-testsuite
    taxi/uservices/userver/testsuite/pytest_plugins/pytest_userver
    taxi/uservices/userver/redis/functional_tests/pytest_redis_cluster_topology_plugin
)

DEPENDS(
    taxi/uservices/userver/redis/functional_tests/cluster_auto_topology
    taxi/uservices/userver/redis/functional_tests/pytest_redis_cluster_topology_plugin/package
)

DATA(
    arcadia/taxi/uservices/userver/redis/functional_tests/cluster_auto_topology
)

CONFTEST_LOAD_POLICY_LOCAL()
TEST_CWD(/)

END()
