import json

import pytest

from testsuite.databases.clickhouse import discover

pytest_plugins = ['pytest_userver.plugins.clickhouse']


@pytest.fixture(scope='session')
def service_env(clickhouse_conn_info) -> dict:
    SECDIST_CONFIG = {
        'clickhouse_settings': {
            'clickhouse-database-alias': {
                'hosts': [clickhouse_conn_info.host],
                'port': clickhouse_conn_info.tcp_port,
                'password': '',
                'user': 'default',
                'dbname': 'clickhouse-database',
            },
        },
    }

    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}


@pytest.fixture(scope='session')
def clickhouse_local(service_source_dir):
    return discover.find_schemas(
        schema_dirs=[service_source_dir.joinpath('schemas', 'clickhouse')],
        dbprefix='',
    )
