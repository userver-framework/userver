import tempfile
import uuid

import pytest


pytest_plugins = ['pytest_userver.plugins.core']


USERVER_CONFIG_HOOKS = ['userver_config_translations']


# redefinition fixture to change log_file path
@pytest.fixture(scope='session')
def userver_config_logging(pytestconfig):
    log_level = pytestconfig.option.service_log_level

    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        assert config_vars['default-log-path'].endswith('.log')
        if 'logging' in components:
            components['logging']['loggers'] = {
                'default': {
                    'file_path': config_vars['default-log-path'],
                    'level': log_level,
                    'overflow_behavior': 'discard',
                },
            }
        config_vars['logger_level'] = log_level

    return _patch_config


@pytest.fixture(scope='session')
def userver_config_translations(mockserver_info):
    def do_patch(config_yaml, config_vars):
        with tempfile.TemporaryDirectory() as tmpdirname:
            log_file_name = tmpdirname + str(uuid.uuid4()) + '.log'
            config_vars['default-log-path'] = log_file_name
            config_yaml['components_manager']['components']['logging'][
                'loggers'
            ]['default']['file_path'] = config_vars['default-log-path']

    return do_patch
