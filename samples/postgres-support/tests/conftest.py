import pathlib

import pytest

from testsuite.databases.pgsql import discover

SERVICE_SOURCE_DIR = pathlib.Path(__file__).parent.parent

pytest_plugins = ['pytest_userver.plugins.postgresql']


@pytest.fixture(scope='session')
def pgsql_local(pgsql_local_create):
    databases = discover.find_schemas(
        'testsuite_support',
        [SERVICE_SOURCE_DIR.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))
