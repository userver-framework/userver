import json

import pytest

from testsuite import utils

pytest_plugins = ['pytest_userver.plugins', 'pytest_userver.plugins.samples']


# /// [config hook]
USERVER_CONFIG_HOOKS = ['userver_config_configs_client']


@pytest.fixture(scope='session')
def userver_config_configs_client(mockserver_info):
    def do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['dynamic-config-client'][
            'config-url'
        ] = mockserver_info.base_url.rstrip('/')

    return do_patch
    # /// [config hook]


@pytest.fixture(autouse=True)
def mock_config_server(mockserver, mocked_time):
    @mockserver.json_handler('/configs/values')
    def mock_configs_values(request):
        return {
            'configs': [],
            'updated_at': utils.timestring(mocked_time.now()),
        }

    return mock_configs_values


@pytest.fixture
def config_default_values(service_source_dir):
    with service_source_dir.joinpath(
            'dynamic_config_fallback.json',
    ).open() as fp:
        return json.load(fp)
