import os

import pytest

pytest_plugins = ['pytest_userver.plugins.grpc']


USERVER_CONFIG_HOOKS = ['static_servicemesh_settings']


@pytest.fixture(scope='session')
def grpc_proxy_endpoint(get_free_port):
    return f'[::]:{get_free_port()}'


@pytest.fixture(scope='session')
def static_servicemesh_settings(grpc_proxy_endpoint, grpc_mockserver_endpoint):
    def hook(config, config_vars):
        os.environ['EGRESS_GRPC_PROXY'] = f'{grpc_proxy_endpoint}'
        components_cfg = config['components_manager']['components']
        components_cfg['grpc-client-common']['servicemesh-settings'] = {
            'egress': {'disable_proxy': [grpc_mockserver_endpoint]},
        }

    return hook
