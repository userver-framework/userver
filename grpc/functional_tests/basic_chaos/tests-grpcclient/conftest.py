# pylint: disable=protected-access,redefined-outer-name
import asyncio
import logging

import grpc
import pytest
from pytest_userver import chaos
import samples.greeter_pb2_grpc as greeter_pb2_grpc  # noqa: E402, E501

from service import GreeterService

logger = logging.getLogger(__name__)

pytest_plugins = ['pytest_userver.plugins.grpc']

USERVER_CONFIG_HOOKS = ['prepare_service_config']


# port for TcpChaos -> client
@pytest.fixture(name='grpc_client_port', scope='session')
def _grpc_client_port(request) -> int:
    # This fixture might be defined in an outer scope.
    if 'for_grpc_server_gate_port' in request.fixturenames:
        return request.getfixturevalue('for_grpc_client_gate_port')
    return 8081


@pytest.fixture(scope='session')
async def _gate_started(loop, grpc_client_port):
    gate_config = chaos.GateRoute(
        name='grpc client tcp proxy',
        host_for_client='localhost',
        port_for_client=0,
        host_to_server='localhost',
        port_to_server=grpc_client_port,
    )
    logger.info(
        f'Create gate client -> ({gate_config.host_for_client}:'
        f'{gate_config.port_for_client}); ({gate_config.host_to_server}:'
        f'{gate_config.port_to_server} -> server)',
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture(scope='session')
def grpc_service_port_local(_gate_started) -> int:
    return int(_gate_started.get_sockname_for_clients()[1])


@pytest.fixture(scope='session')
def prepare_service_config(grpc_service_port_local):
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        components['greeter-client'][
            'endpoint'
        ] = f'[::]:{grpc_service_port_local}'

    return patch_config


@pytest.fixture
def extra_client_deps(_gate_started):
    pass


@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate_started):
    _gate_started.to_server_pass()
    _gate_started.to_client_pass()
    _gate_started.start_accepting()

    await _gate_started.sockets_close()

    yield _gate_started


@pytest.fixture(scope='session')
async def server_run(grpc_client_port):
    server = grpc.aio.server()
    greeter_pb2_grpc.add_GreeterServiceServicer_to_server(
        GreeterService(), server,
    )
    listen_addr = f'[::]:{grpc_client_port}'
    server.add_insecure_port(listen_addr)
    logging.info('Starting server on %s', listen_addr)
    server_task = asyncio.create_task(server.start())
    try:
        yield server
    finally:
        await server.stop(grace=None)
        await server.wait_for_termination()
        await server_task


@pytest.fixture(scope='session')
async def _grpc_session_ch(server_run, grpc_service_port_local):
    async with grpc.aio.insecure_channel(
            f'localhost:{grpc_service_port_local}',
    ) as channel:
        yield channel


@pytest.fixture(scope='session')
async def grpc_ch(_grpc_session_ch, grpc_service_port_local):
    try:
        await asyncio.wait_for(_grpc_session_ch.channel_ready(), timeout=10)
    except asyncio.TimeoutError:
        raise RuntimeError(
            'Failed to connect to remote gRPC server by '
            f'address localhost:{grpc_service_port_local}',
        )
    return _grpc_session_ch
