import pathlib

import pytest
import pytest_userver.config

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
@pytest_userver.config.patch
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
