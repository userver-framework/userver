import json

import pytest

from testsuite.databases.mysql import discover

pytest_plugins = [
    'pytest_userver.plugins',
    'pytest_userver.plugins.samples',
    # Database related plugins
    'testsuite.databases.mysql.pytest_plugin',
]


SECDIST_CONFIG = {
    # /// [Clickhouse service sample - secdist]
    # json
    'mysql_settings': {
        'test': {
            'hosts': ['localhost'],
            'port': 13307,
            'password': '',
            'user': 'root',
            'database': 'test',
        },
    },
    # /// [Clickhouse service sample - secdist]
}


@pytest.fixture(scope='session')
def service_env():
    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}


@pytest.fixture(scope='session')
def mysql_local(service_source_dir):
    databases = discover.find_schemas(
        schema_dirs=[service_source_dir.joinpath('schemas', 'mysql')],
        dbprefix=''
    )

    return databases


@pytest.fixture
def client_deps(mysql):
    pass
