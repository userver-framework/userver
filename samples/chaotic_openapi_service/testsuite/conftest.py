import pytest

pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['userver_config_client']


# /// [URL]
@pytest.fixture(scope='session')
def userver_config_client(mockserver_info):
    def do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['test-client']['base-url'] = mockserver_info.url('test')

    return do_patch
    # /// [URL]
