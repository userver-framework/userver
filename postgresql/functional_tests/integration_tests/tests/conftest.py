import pytest

from testsuite.databases.pgsql import discover


pytest_plugins = [
    'pytest_userver.plugins.config',
    'pytest_userver.plugins.dynamic_config',
    'pytest_userver.plugins.service',
    'pytest_userver.plugins.service_client',
    'taxi.integration_testing.pytest_plugin',
    'taxi.uservices.testsuite.integration_testing.pytest_plugin',
]


USERVER_CONFIG_HOOKS = ['userver_pg_config']


@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    databases = discover.find_schemas(
        'pg', [service_source_dir.joinpath('schemas/postgresql')],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture(scope='session')
def userver_pg_config(_pgsql_service_settings):
    def _hook_db_config(config_yaml, config_vars):
        settings = _pgsql_service_settings.get_conninfo()

        components = config_yaml['components_manager']['components']
        db = components['key-value-database']
        db['dbconnection'] = (
            'postgresql://'
            f'{settings.user}:{settings.password}@'
            f'{settings.host}:{settings.port}'
            '/pg_key_value'
        )

    return _hook_db_config
