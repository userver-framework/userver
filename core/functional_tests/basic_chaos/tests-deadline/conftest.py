import pytest

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
def userver_testsuite_middleware_enabled():
    return False


@pytest.fixture(name='userver_config_http_client', scope='session')
def _userver_config_http_client(userver_config_http_client):
    def patch_config(config_yaml, config_vars) -> None:
        userver_config_http_client(config_yaml, config_vars)

        components = config_yaml['components_manager']['components']
        http_client = components['http-client']
        # There are tests in this module that specifically want to force
        # http-client timeouts.
        http_client.pop('testsuite-timeout')

    return patch_config
