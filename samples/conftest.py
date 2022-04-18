import pathlib

import pytest
from testsuite.utils import url_util

# /// [testsuite - pytest_plugins]
pytest_plugins = [
    'pytest_userver.plugins',
    # Database related plugins
    'testsuite.databases.mongo.pytest_plugin',
    'testsuite.databases.pgsql.pytest_plugin',
    'testsuite.databases.redis.pytest_plugin',
    'testsuite.databases.clickhouse.pytest_plugin',
]
# /// [testsuite - pytest_plugins]
USERVER_CONFIG_HOOKS = ['sample_config_hook']


@pytest.fixture(scope='session')
def test_service_env():
    return None


@pytest.fixture(scope='session')
async def service_daemon(
        pytestconfig,
        create_daemon_scope,
        test_service_env,
        service_baseurl,
        service_config_path,
        service_config_yaml,
        create_port_health_checker,
):
    components = service_config_yaml['components_manager']['components']

    ping_url = None
    health_check = None

    if 'handler-ping' in components:
        ping_url = url_util.join(
            service_baseurl, components['handler-ping']['path'],
        )
    else:
        health_check = create_port_health_checker(
            hostname='localhost', port=pytestconfig.option.service_port,
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
