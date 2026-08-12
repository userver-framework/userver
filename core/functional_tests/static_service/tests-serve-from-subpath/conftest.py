import pathlib

import pytest

pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['static_config_hook']


@pytest.fixture(scope='session')
def static_config_hook(service_source_dir):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        if 'fs-cache-main' in components:
            components['fs-cache-main']['dir'] = str(
                pathlib.Path(service_source_dir).joinpath('public'),
            )

        if 'handler-static' in components:
            components['handler-static']['path'] = '/possible/to/work/from/subpath/*'

    return _patch_config
