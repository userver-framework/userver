import typing

import pytest

pytest_plugins = ['pytest_userver.plugins.core']


# For dynamic_configs/test_fixtures.py
@pytest.fixture(scope='session')
def dynamic_config_fallback_patch() -> typing.Dict[str, typing.Any]:
    return {'HTTP_CLIENT_CONNECTION_POOL_SIZE': 777}


# redefinition fixture to change log_file path
@pytest.fixture(name='userver_config_logging', scope='session')
def _userver_config_logging(userver_config_logging, service_tmpdir):
    def _patch_config(config_yaml, config_vars):
        userver_config_logging(config_yaml, config_vars)
        components = config_yaml['components_manager']['components']
        assert 'logging' in components
        components['logging']['loggers'] = {
            'default': {'file_path': str(service_tmpdir / 'server.log')},
        }

    return _patch_config
