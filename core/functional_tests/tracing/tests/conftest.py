import pytest


pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['config_echo_url']


@pytest.fixture(scope='session')
def config_echo_url(mockserver_info):
    def _do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['handler-echo-no-body']['echo-url'] = mockserver_info.url(
            '/test-service/echo-no-body',
        )

    return _do_patch
