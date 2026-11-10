import copy
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
def service_env(mysql_conninfo):
    secdist_config = copy.deepcopy(SECDIST_CONFIG)
    secdist_config['mysql_settings']['sample-sql-component'].update(
        hosts=[mysql_conninfo.hostname],
        port=mysql_conninfo.port,
        user=mysql_conninfo.user,
        password=mysql_conninfo.password or '',
    )
    return {'SECDIST_CONFIG': json.dumps(secdist_config)}


@pytest.fixture(scope='session')
def mysql_local(service_source_dir):
    databases = discover.find_schemas(
        schema_dirs=[service_source_dir.joinpath('schemas', 'mysql')],
        dbprefix='',
    )

    return databases
