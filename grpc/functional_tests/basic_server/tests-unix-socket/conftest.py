import pathlib
import tempfile
from typing import Iterator

import pytest

import samples.greeter_pb2_grpc as greeter_services

pytest_plugins = ['pytest_userver.plugins.grpc']

USERVER_CONFIG_HOOKS = ['config_echo_url']


@pytest.fixture(scope='session')
def config_echo_url(mockserver_info):
    def _do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['greeter-service']['echo-url'] = mockserver_info.url(
            '/test-service/echo-no-body',
        )

    return _do_patch


@pytest.fixture(scope='session')
def unix_socket_path(tmp_path_factory) -> Iterator[pathlib.Path]:
    with tempfile.TemporaryDirectory(prefix='userver-grpc-socket-') as name:
        yield pathlib.Path(name) / 's'


# Overrides userver fixture.
@pytest.fixture(scope='session')
def userver_config_grpc_endpoint(unix_socket_path):
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        components['grpc-server']['unix-socket-path'] = str(unix_socket_path)

    return patch_config


@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_services.GreeterServiceStub(grpc_channel)
