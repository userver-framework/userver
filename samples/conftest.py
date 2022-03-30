import pathlib

import pytest
from testsuite.daemons import service_client
from testsuite.utils import url_util

pytest_plugins = [
    'pytest_userver',
    'testsuite.databases.mongo.pytest_plugin',
    'testsuite.databases.pgsql.pytest_plugin',
    'testsuite.databases.redis.pytest_plugin',
    'testsuite.databases.clickhouse.pytest_plugin',
]
USERVER_CONFIG_HOOKS = ['sample_config_hook']


@pytest.fixture
def client_deps():
    pass


@pytest.fixture
async def test_service_client(
        ensure_daemon_started,
        service_client_options,
        mockserver,
        test_service_daemon,
        test_service_baseurl,
        client_deps,
):
    await ensure_daemon_started(test_service_daemon)
    return service_client.Client(
        test_service_baseurl, **service_client_options,
    )


@pytest.fixture
def test_monitor_client(
        test_service_client,
        test_monitor_baseurl,
        service_client_options,
        mockserver,
):
    return service_client.Client(
        test_monitor_baseurl,
        headers={'x-yatraceid': mockserver.trace_id},
        **service_client_options,
    )


@pytest.fixture(scope='session')
def test_service_baseurl(pytestconfig):
    return f'http://localhost:{pytestconfig.option.test_service_port}/'


@pytest.fixture(scope='session')
def test_monitor_baseurl(pytestconfig):
    return f'http://localhost:{pytestconfig.option.test_monitor_port}/'


@pytest.fixture(scope='session')
def test_service_env():
    return None


@pytest.fixture(scope='session')
async def test_service_daemon(
        pytestconfig,
        create_daemon_scope,
        test_service_baseurl,
        test_service_env,
        service_config_path,
        service_config_yaml,
        create_port_health_checker,
):
    components = service_config_yaml['components_manager']['components']

    ping_url = None
    health_check = None

    if 'handler-ping' in components:
        ping_url = url_util.join(
            test_service_baseurl, components['handler-ping']['path'],
        )
    else:
        health_check = create_port_health_checker(
            hostname='localhost', port=pytestconfig.option.test_service_port,
        )

    async with create_daemon_scope(
            args=[
                str(pytestconfig.option.service_binary),
                '--config',
                str(service_config_path),
            ],
            ping_url=ping_url,
            health_check=health_check,
            env=test_service_env,
    ) as scope:
        yield scope


@pytest.fixture(scope='session')
def sample_config_hook(service_source_dir: pathlib.Path):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        if 'secdist' in components:
            components['secdist']['config'] = str(
                service_source_dir.joinpath('secure_data.json'),
            )

        if 'taxi-config-fallbacks' in components:
            components['taxi-config-fallbacks']['fallback-path'] = str(
                service_source_dir.joinpath('dynamic_config_fallback.json'),
            )

        if 'taxi-config-client-updater' in components:
            components['taxi-config-client-updater']['fallback-path'] = str(
                service_source_dir.joinpath('dynamic_config_fallback.json'),
            )

    return _patch_config
