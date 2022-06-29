import pathlib

import pytest

from testsuite.databases.pgsql import discover

SERVICE_SOURCE_DIR = pathlib.Path(__file__).parent.parent
USERVER_CONFIG_HOOKS = ['pgsql_config_hook']

# /// [testsuite - pytest_plugins]
pytest_plugins = [
    'pytest_userver.plugins',
    'testsuite.databases.pgsql.pytest_plugin',
]
# /// [testsuite - pytest_plugins]


@pytest.fixture(scope='session')
def pgsql_local(pgsql_local_create):
    databases = discover.find_schemas(
        'testsuite_support',
        [SERVICE_SOURCE_DIR.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture
def client_deps(pgsql):
    pass


@pytest.fixture(scope='session')
def pgsql_config_hook(pgsql_local):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['postgresql-service']['dbconnection'] = pgsql_local[
            'service'
        ].get_uri()

    return _patch_config
