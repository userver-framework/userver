import pathlib

import pytest
import pytest_userver.config

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture(scope='session')
@pytest_userver.config.patch
def static_servicemesh_settings(testsuite_get_source_path):
    tests_dir = testsuite_get_source_path(pathlib.Path(__file__).parent)
    service_mesh_file = tests_dir / 'static' / 'service_mesh_settings.yaml'

    def hook(config, config_vars):
        components_cfg = config['components_manager']['components']
        components_cfg['grpc-client-common']['servicemesh-settings#file'] = str(service_mesh_file.absolute())

    return hook


@pytest.fixture(scope='session')
def service_env():
    return {
        'EGRESS_GRPC_PROXY': '',
    }
