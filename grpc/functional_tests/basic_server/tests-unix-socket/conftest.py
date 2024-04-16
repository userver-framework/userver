import pathlib

import pytest
import samples.greeter_pb2_grpc as greeter_services  # noqa: E402, E501

pytest_plugins = ['pytest_userver.plugins.grpc']

USERVER_CONFIG_HOOKS = ['prepare_service_config']


@pytest.fixture(scope='session')
def unix_socket_path(tmp_path_factory) -> pathlib.Path:
    return tmp_path_factory.mktemp('socket_dir') / 's'


@pytest.fixture(scope='session')
def grpc_service_endpoint(service_config) -> str:
    components = service_config['components_manager']['components']
    return f'unix:{components["grpc-server"]["unix-socket-path"]}'


@pytest.fixture(name='prepare_service_config', scope='session')
def _prepare_service_config(unix_socket_path):
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        components['grpc-server']['unix-socket-path'] = str(unix_socket_path)

    return patch_config


@pytest.fixture
def grpc_client(grpc_channel, service_client):
    return greeter_services.GreeterServiceStub(grpc_channel)
