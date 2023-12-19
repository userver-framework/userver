# pylint: disable=protected-access
import logging

import pytest
from pytest_userver import chaos
import samples.greeter_pb2_grpc as greeter_pb2_grpc  # noqa: E402, E501


logger = logging.getLogger(__name__)

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
async def _gate_started(loop, grpc_server_port):
    gate_config = chaos.GateRoute(
        name='grpc tcp proxy',
        host_for_client='localhost',
        port_for_client=0,
        host_to_server='localhost',
        port_to_server=grpc_server_port,
    )
    logger.info(
        f'Create gate client -> ({gate_config.host_for_client}:'
        f'{gate_config.port_for_client}); ({gate_config.host_to_server}:'
        f'{gate_config.port_to_server} -> server)',
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate_started):
    _gate_started.to_server_pass()
    _gate_started.to_client_pass()
    _gate_started.start_accepting()

    yield _gate_started


@pytest.fixture
def extra_client_deps(_gate_started):
    pass


@pytest.fixture(scope='session')
def grpc_service_port(_gate_started) -> int:
    return int(_gate_started.get_sockname_for_clients()[1])


@pytest.fixture
def grpc_client(grpc_channel, service_client, gate):
    return greeter_pb2_grpc.GreeterServiceStub(grpc_channel)


@pytest.fixture(scope='session')
def prepare_service_config(grpc_server_port):
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        components['grpc-server']['port'] = grpc_server_port

    return patch_config
