# pylint: disable=redefined-outer-name
import pathlib
import sys

import healthchecking.healthchecking_pb2_grpc as healthchecking_pb2_grpc
import pytest


pytest_plugins = ['pytest_userver.plugins.grpc']

USERVER_CONFIG_HOOKS = ['prepare_service_config']


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
            / 'grpc/handlers/proto/healthchecking',
        ),
    )


@pytest.fixture(scope='function')
def grpc_client(grpc_channel, service_client):
    return healthchecking_pb2_grpc.HealthStub(grpc_channel)
