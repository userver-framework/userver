import os
import pathlib

import pytest

pytest_plugins = ['pytest_userver.plugins.grpc']


USERVER_CONFIG_HOOKS = ['static_servicemesh_settings']


@pytest.fixture(scope='session')
def static_servicemesh_settings():
    def hook(config, config_vars):
        os.environ['EGRESS_GRPC_PROXY'] = ''
        path = pathlib.Path('static/service-mesh/service_settings.yaml')
        components_cfg = config['components_manager']['components']
        components_cfg['grpc-client-common']['servicemesh-settings#file'] = str(path.absolute())

    return hook
