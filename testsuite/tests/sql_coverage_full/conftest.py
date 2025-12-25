import pytest

pytest_plugins = ['pytest_userver.plugins.core', 'pytest_userver.plugins.sql_coverage']


@pytest.fixture(scope='session')
def sql_files() -> set:
    return {'query1', 'query2', 'query3'}
