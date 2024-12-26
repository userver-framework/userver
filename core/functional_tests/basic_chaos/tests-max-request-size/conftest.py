import pytest

pytest_plugins = ['pytest_userver.plugins.core', 'pytest_userver.plugins']


@pytest.fixture(name='userver_config_http_client', scope='session')
def _userver_config_http_client(userver_config_http_client):
    def patch_config(config_yaml, config_vars) -> None:
        userver_config_http_client(config_yaml, config_vars)

        components = config_yaml['components_manager']['components']
        server = components['server']
        server['listener']['handler-defaults'] = {'max_request_size': 1000}

    return patch_config
