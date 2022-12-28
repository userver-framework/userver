import pytest

from testsuite.databases.pgsql import discover

pytest_plugins = [
    'pytest_userver.plugins',
    'pytest_userver.plugins.samples',
    'testsuite.databases.pgsql.pytest_plugin',
]


USERVER_CONFIG_HOOKS = ['pgsql_config_hook']


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'auth', [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture
def client_deps(pgsql):
    pass


@pytest.fixture(scope='session')
def pgsql_config_hook(pgsql_local):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['auth-database']['dbconnection'] = pgsql_local[
            'auth'
        ].get_uri()

    return _patch_config
