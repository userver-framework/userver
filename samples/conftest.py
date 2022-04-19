import pathlib

import pytest

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
def sample_config_hook(service_source_dir: pathlib.Path):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        if 'secdist' in components:
            components['secdist']['config'] = str(
                service_source_dir.joinpath('secure_data.json'),
            )

    return _patch_config
