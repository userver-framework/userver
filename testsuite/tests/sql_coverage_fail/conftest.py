import pytest

pytest_plugins = ['pytest_userver.plugins.core', 'pytest_userver.plugins.sql_coverage']


@pytest.fixture(scope='session')
def sql_files() -> set:
    return set(['query1', 'query2', 'query3'])


@pytest.fixture(scope='session')
def counter_increment():
    counter = 0

    def increment():
        nonlocal counter
        counter += 1

    yield increment
    assert counter == 1
