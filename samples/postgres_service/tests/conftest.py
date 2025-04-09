# /// [psql prepare]
import pytest

from testsuite.databases.pgsql import discover

pytest_plugins = ['pytest_userver.plugins.postgresql', 'pytest_userver.plugins.sql_coverage']


@pytest.fixture(scope='session')
def sql_files() -> set:
    return set(['sample_insert_value', 'sample_select_value'])


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'admin',
        [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))
    # /// [psql prepare]
