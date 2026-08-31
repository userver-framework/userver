import json
import pathlib

import pytest

pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['userver_config_client', 'userver_config_secure_client']
SERVICE_SOURCE_DIR = pathlib.Path(__file__).parent.parent


@pytest.fixture(scope='session')
def service_secdist_path():
    return SERVICE_SOURCE_DIR / 'secure_data.json'


# /// [URL]
@pytest.fixture(scope='session')
def userver_config_client(mockserver_info):
    def do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['test-client']['base-url'] = mockserver_info.url('test')

    return do_patch
    # /// [URL]


@pytest.fixture(scope='session')
def service_env():
    secdist_config = {
        'tokens': {
            'user-1-token': 123,
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}


# /// [digest-client-url]
@pytest.fixture(scope='session')
def userver_config_secure_client(mockserver_info):
    def do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['secure-client']['base-url'] = mockserver_info.url('api')

    return do_patch
    # /// [digest-client-url]
