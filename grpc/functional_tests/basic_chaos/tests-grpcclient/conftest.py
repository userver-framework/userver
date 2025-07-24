import asyncio
import logging

import grpc
import pytest
from pytest_userver import chaos
from service import GreeterService

logger = logging.getLogger(__name__)

pytest_plugins = ['pytest_userver.plugins.grpc']

USERVER_CONFIG_HOOKS = ['prepare_service_config']


# port for TcpChaos -> client
@pytest.fixture(name='grpc_client_port', scope='session')
def _grpc_client_port(request, get_free_port) -> int:
    return get_free_port()


@pytest.fixture(scope='session')
async def _gate_started(grpc_client_port, grpc_mockserver_session):
    gate_config = chaos.GateRoute(
        name='grpc client tcp proxy',
        host_for_client='::1',
        port_for_client=0,
        host_to_server='::1',
        port_to_server=grpc_client_port,
    )
    logger.info(
        f'Create gate client -> ({gate_config.host_for_client}:'
        f'{gate_config.port_for_client}); ({gate_config.host_to_server}:'
        f'{gate_config.port_to_server} -> server)',
    )
    async with chaos.TcpGate(gate_config) as proxy:
        yield proxy


@pytest.fixture(scope='session')
def grpc_service_port_local(_gate_started) -> int:
    return int(_gate_started.get_sockname_for_clients()[1])


@pytest.fixture(scope='session')
def prepare_service_config(grpc_service_port_local):
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        components['greeter-client']['endpoint'] = f'[::]:{grpc_service_port_local}'

    return patch_config


@pytest.fixture
def extra_client_deps(gate):
    pass


@pytest.fixture(name='gate')
async def _gate_ready(_gate_started, greeter_mock):
    await _gate_started.to_server_pass()
    await _gate_started.to_client_pass()
    _gate_started.start_accepting()

    await _gate_started.sockets_close()

    yield _gate_started


# [grpc_mockserver_endpoint example]
# Overriding userver fixture
@pytest.fixture(scope='session')
def grpc_mockserver_endpoint(grpc_client_port):
    return f'[::]:{grpc_client_port}'


# [grpc_mockserver_endpoint example]


# [installing mockserver servicer]
@pytest.fixture(name='greeter_mock')
def _greeter_mock(grpc_mockserver):
    mock = GreeterService()
    grpc_mockserver.install_servicer(mock)
    return mock
    # [installing mockserver servicer]


@pytest.fixture(scope='session')
async def _grpc_session_ch(grpc_mockserver_session, grpc_service_port_local):
    async with grpc.aio.insecure_channel(f'[::1]:{grpc_service_port_local}') as channel:
        yield channel


@pytest.fixture(scope='session')
async def grpc_ch(_grpc_session_ch, grpc_service_port_local):
    try:
        await asyncio.wait_for(_grpc_session_ch.channel_ready(), timeout=10)
    except asyncio.TimeoutError:
        raise RuntimeError(
            f'Failed to connect to remote gRPC server by address [::1]:{grpc_service_port_local}',
        )
    return _grpc_session_ch
