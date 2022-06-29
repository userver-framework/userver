import json

import pytest

from testsuite.databases.clickhouse import discover

pytest_plugins = [
    'pytest_userver.plugins',
    'pytest_userver.plugins.samples',
    # Database related plugins
    'testsuite.databases.clickhouse.pytest_plugin',
]


SECDIST_CONFIG = {
    # /// [Clickhouse service sample - secdist]
    # json
    'clickhouse_settings': {
        'clickhouse-database-alias': {
            'hosts': ['localhost'],
            'port': 17123,
            'password': '',
            'user': 'default',
            'dbname': 'clickhouse-database',
        },
    },
    # /// [Clickhouse service sample - secdist]
}


@pytest.fixture(scope='session')
def service_env():
    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}


@pytest.fixture(scope='session')
def clickhouse_local(service_source_dir):
    return discover.find_schemas(
        schema_dirs=[service_source_dir.joinpath('schemas', 'clickhouse')],
        dbprefix='',
    )


@pytest.fixture
def client_deps(clickhouse):
    pass
