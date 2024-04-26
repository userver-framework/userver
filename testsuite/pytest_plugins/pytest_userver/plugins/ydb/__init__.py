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


def pytest_configure(config):
    config.addinivalue_line(
        'markers', 'ydb: per-test ydb-local initialization',
    )
