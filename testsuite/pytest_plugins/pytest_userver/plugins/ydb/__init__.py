"""
pytest plugin that provides YDB fixtures for functional tests with
testsuite; see
@ref scripts/docs/en/userver/ydb.md for an introduction.

@ingroup userver_testsuite_fixtures
"""

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.ydb.ydbsupport',
]


def pytest_addoption(parser):
    group = parser.getgroup('ydb')
    group.addoption('--ydb-host', help='YDB host')
    group.addoption('--ydb-grpc-port', type=int, help='YDB grpc host')
    group.addoption('--ydb-mon-port', type=int, help='YDB mon host')
    group.addoption('--ydb-ic-port', type=int, help='YDB ic host')
    group.addoption('--ydb-wait-time', type=int, default=30, help='YDB container startup wait timeout in seconds (0 = no wait)')


def pytest_configure(config):
    config.addinivalue_line(
        'markers',
        'ydb: per-test ydb-local initialization',
    )
