import pytest

from testsuite.databases.pgsql import discover

pytest_plugins = ['pytest_userver.plugins.postgresql']


@pytest.fixture(scope='session')
def pgsql_local(db_dump_schema_path, pgsql_local_create):
    databases = discover.find_schemas('admin', [db_dump_schema_path])
    return pgsql_local_create(list(databases.values()))
