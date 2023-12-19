import pytest

from testsuite.databases.mysql import discover

pytest_plugins = ['testsuite.databases.pgsql.pytest_plugin']


@pytest.fixture(scope='session')
def mysql_local(service_source_dir):
    return discover.find_schemas(
        [service_source_dir.joinpath('schemas')], dbprefix='',
    )
