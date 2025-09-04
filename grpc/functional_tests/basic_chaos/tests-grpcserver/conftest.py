import ipaddress
import logging

import pytest
from pytest_userver import chaos

import samples.greeter_pb2_grpc as greeter_pb2_grpc

logger = logging.getLogger(__name__)

pytest_plugins = ['pytest_userver.plugins.grpc']


# port for TcpChaos -> server
@pytest.fixture(name='uservice_grpc_server_port', scope='session')
def _uservice_grpc_server_port(choose_free_port) -> int:
    return choose_free_port(8091)


@pytest.fixture(scope='session')
async def _gate_started(uservice_grpc_server_port):
    gate_config = chaos.GateRoute(
        name='grpc tcp proxy',
        host_for_client='localhost',
        port_for_client=0,
        host_to_server='localhost',
        port_to_server=uservice_grpc_server_port,
    )
    logger.info(
        f'Create gate: client -> (gate {gate_config.host_for_client}:{gate_config.port_for_client}) -> '
        f'(uservice {gate_config.host_to_server}:{gate_config.port_to_server})',
    )
    async with chaos.TcpGate(gate_config) as proxy:
        yield proxy


@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate_started):
    await _gate_started.to_server_pass()
    await _gate_started.to_client_pass()
    _gate_started.start_accepting()

    yield _gate_started


@pytest.fixture
def extra_client_deps(_gate_started):
    pass


@pytest.fixture(scope='session')
def grpc_service_endpoint(_gate_started) -> str:
    """Endpoint used by `grpc_channel`. This is the client-facing endpoint of chaos proxy."""
    host, port = _gate_started.get_sockname_for_clients()
    if ipaddress.ip_address(host).version == 6:
        return f'[{host}]:{port}'
    return f'{host}:{port}'


@pytest.fixture
def grpc_client(grpc_channel, service_client, gate):
    return greeter_pb2_grpc.GreeterServiceStub(grpc_channel)


# Overrides userver fixture.
@pytest.fixture(scope='session')
def userver_config_grpc_endpoint(uservice_grpc_server_port):
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        components['grpc-server']['port'] = uservice_grpc_server_port

    return patch_config
