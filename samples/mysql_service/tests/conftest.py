import json

import pytest

from testsuite.databases.mysql import discover

pytest_plugins = ['pytest_userver.plugins.mysql']


SECDIST_CONFIG = {
    # /// [Mysql service sample - secdist]
    'mysql_settings': {
        'sample-sql-component': {
            # First host is considered a primary, all others - secondaries.
            # This behavior is hard-coded and the mysql driver doesn't perform
            # automatic primary [re]discovery.
            'hosts': ['localhost'],
            'port': 13307,
            'password': '',
            'user': 'root',
            'database': 'sample_db',
        },
    },
    # /// [Mysql service sample - secdist]
}


@pytest.fixture(scope='session')
def service_env():
    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}


@pytest.fixture(scope='session')
def mysql_local(service_source_dir):
    databases = discover.find_schemas(
        schema_dirs=[service_source_dir.joinpath('schemas', 'mysql')],
        dbprefix='',
    )

    return databases
