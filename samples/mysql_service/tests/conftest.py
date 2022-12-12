import json

import pytest

from testsuite.databases.mysql import discover

pytest_plugins = [
    'pytest_userver.plugins',
    'pytest_userver.plugins.samples',
    # Database related plugins
    'testsuite.databases.mysql.pytest_plugin',
]


@pytest.fixture(scope='session')
def mysql_local(service_source_dir):
    databases = discover.find_schemas(
        schema_dirs=[service_source_dir.joinpath('schemas', 'mysql')],
        dbprefix=''
    )

    print(databases)

    return databases


@pytest.fixture
def client_deps(mysql):
    pass
