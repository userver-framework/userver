import pytest

pytest_plugins = ['pytest_userver.plugins.core', 'pytest_userver.plugins']


@pytest.fixture(scope='session')
def port1(get_free_port):
    return get_free_port()


@pytest.fixture(scope='session')
def port2(get_free_port):
    return get_free_port()


@pytest.fixture(name='userver_config_http_client', scope='session')
def _userver_config_http_client(userver_config_http_client, port1, port2):
    def patch_config(config_yaml, config_vars) -> None:
        userver_config_http_client(config_yaml, config_vars)

        server = config_yaml['components_manager']['components']['server']
        server['listener']['ports'] = [{'port': port1}, {'port': port2}]

    return patch_config
