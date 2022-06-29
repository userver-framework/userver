import pytest

# /// [psql prepare]
from testsuite.databases.pgsql import discover

pytest_plugins = [
    'pytest_userver.plugins',
    'pytest_userver.plugins.samples',
    # Database related plugins
    'testsuite.databases.pgsql.pytest_plugin',
]


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'admin', [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))
    # /// [psql prepare]


# /// [client_deps]
@pytest.fixture
def client_deps(pgsql):
    pass
    # /// [client_deps]
