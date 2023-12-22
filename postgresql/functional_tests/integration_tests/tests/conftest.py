import pathlib

import pytest
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
    'pytest_userver.plugins.postgresql',
]


USERVER_CONFIG_HOOKS = ['userver_pg_config']


@pytest_asyncio.fixture(name='testenv', scope='session')
async def _testenv(
        service_source_dir: pathlib.Path, request: pytest.FixtureRequest,
) -> it.Environment:
    env = it.Environment(
        {
            **it.CORE,
            **it.DOCKER,
            'database_common': it.databases.DatabaseCommon(),
            **it.mockserver.create_mockserver(),
            **it.databases.pgsql.create_pgsql(replicas=1),
        },
    )
    env.configure(it.PytestConfig(request.config))
    env.pgsql.discover_schemas(
        service_source_dir.joinpath('schemas/postgresql'), service_name='pg',
    )
    async with env.run():
        import time
        time.sleep(60)
        yield env


@pytest_asyncio.fixture(scope='session')
async def userver_pg_config(testenv: it.Environment):
    settings = await testenv.pgsql_primary_container.get_conn_info()
    settings = settings.replace(host='127.0.0.1')

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
