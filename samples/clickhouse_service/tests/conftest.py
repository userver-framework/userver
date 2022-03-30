import json

import pytest

from testsuite.databases.clickhouse import discover


SECDIST_CONFIG = {
    'clickhouse_settings': {
        'clickhouse-database': {
            'hosts': ['localhost'],
            'port': 17123,
            'password': '',
            'user': 'default',
        },
    },
}


@pytest.fixture(scope='session')
def test_service_env():
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
