import pathlib

import pytest_asyncio

import taxi.integration_testing as it


pytest_plugins = [
    'testsuite.pytest_plugin',
    'pytest_userver.plugins.caches',
    'pytest_userver.plugins.config',
    'pytest_userver.plugins.dumps',
    'pytest_userver.plugins.dynamic_config',
    'pytest_userver.plugins.log_capture',
    'pytest_userver.plugins.service',
    'pytest_userver.plugins.service_client',
    'pytest_userver.plugins.testpoint',
]


USERVER_CONFIG_HOOKS = ['userver_pg_config']


@pytest_asyncio.fixture(scope='session')
async def testenv(
        service_source_dir: pathlib.Path,
) -> it.databases.pgsql.EnvPgsqlDocker:
    env = it.databases.pgsql.EnvPgsqlDocker(pgsql_replicas=1)
    env.pgsql.discover_schemas(
        service_source_dir.joinpath('schemas/postgresql'), service_name='pg',
    )
    async with env.run():
        yield env


@pytest_asyncio.fixture(scope='session')
async def userver_pg_config(testenv: it.databases.pgsql.EnvPgsqlDocker):
    settings = await testenv.get_pgsql_conn_info()

    def _hook_db_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        db = components['key-value-database']
        db['dbconnection'] = (
            'postgresql://'
            f'{settings.user}:{settings.password}@'
            f'{settings.host}:{settings.port}'
            '/pg_key_value'
        )

    return _hook_db_config
