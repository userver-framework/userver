import json

import pytest

from testsuite import utils


@pytest.fixture(scope='session')
def patch_service_yaml(mockserver_info):
    def do_patch(config_yaml):
        components = config_yaml['components_manager']['components']
        components['taxi-configs-client'][
            'config-url'
        ] = mockserver_info.base_url.rstrip('/')

    return do_patch


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
