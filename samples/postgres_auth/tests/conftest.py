import pytest

from testsuite.databases.pgsql import discover

pytest_plugins = [
    'pytest_userver.plugins',
    'pytest_userver.plugins.samples',
    'testsuite.databases.pgsql.pytest_plugin',
    'pytest_userver.plugins.pg_conf',
]


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'auth', [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture
def client_deps(pgsql):
    pass
