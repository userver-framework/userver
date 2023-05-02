# pylint: disable=redefined-outer-name
import pathlib
import sys

import grpc
import pytest

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins',
    'pytest_userver.plugins.grpc',
]

USERVER_CONFIG_HOOKS = ['prepare_service_config']


# port for client -> TcpChaos
@pytest.fixture(name='for_client_gate_port', scope='session')
def _for_client_gate_port(request) -> int:
    # This fixture might be defined in an outer scope.
    if 'for_grpc_client_gate_port' in request.fixturenames:
        return request.getfixturevalue('for_grpc_client_gate_port')
    return 8099


# port for TcpChaos -> server
@pytest.fixture(name='grpc_server_port', scope='session')
def _grpc_server_port(request) -> int:
    # This fixture might be defined in an outer scope.
    if 'for_grpc_server_gate_port' in request.fixturenames:
        return request.getfixturevalue('for_grpc_server_gate_port')
    return 8091


@pytest.fixture(scope='session')
def prepare_service_config(grpc_server_port):
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        components['grpc-server']['port'] = grpc_server_port

    return patch_config


def pytest_configure(config):
    sys.path.append(
        str(
            pathlib.Path(config.rootdir)
            / 'grpc/functional_tests/basic_chaos/proto/samples',
        ),
    )


@pytest.fixture(scope='session')
def greeter_protos():
    return grpc.protos('greeter.proto')


@pytest.fixture(scope='session')
def greeter_services():
    return grpc.services('greeter.proto')


@pytest.fixture(scope='function')
def grpc_client(grpc_channel, greeter_services, service_client):
    return greeter_services.GreeterServiceStub(grpc_channel)
