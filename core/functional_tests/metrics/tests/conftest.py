import pytest


pytest_plugins = ['pytest_userver.plugins.core']


# redefinition fixture to change log_file path
@pytest.fixture(scope='session')
def userver_config_logging(userver_config_logging, service_tmpdir):
    def _patch_config(config_yaml, config_vars):
        userver_config_logging(config_yaml, config_vars)
        components = config_yaml['components_manager']['components']
        assert 'logging' in components
        components['logging']['loggers'] = {
            'default': {'file_path': str(service_tmpdir / 'server.log')},
        }

    return _patch_config
