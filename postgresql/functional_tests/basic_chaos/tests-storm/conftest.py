import pytest


@pytest.fixture(name='userver_config_testsuite', scope='session')
def _userver_config_testsuite(userver_config_testsuite):
    def patch_config(config_yaml, config_vars):
        userver_config_testsuite(config_yaml, config_vars)
        config_yaml['components_manager'].setdefault('userver_experiments', {})['pg-connecting-rate-limit'] = True

    return patch_config
