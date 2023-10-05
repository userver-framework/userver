import pytest


pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['userver_config_echo_url']


@pytest.fixture(scope='session')
def userver_config_echo_url(mockserver_info):
    def do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['handler-echo-no-body']['echo-url'] = mockserver_info.url(
            '/test-service/echo-no-body',
        )

    return do_patch


@pytest.fixture
def taxi_test_service(service_client):
    return service_client
