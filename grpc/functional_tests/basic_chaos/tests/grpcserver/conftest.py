# pylint: disable=protected-access
import logging

import pytest
from pytest_userver import chaos


logger = logging.getLogger(__name__)


@pytest.fixture(scope='module')
async def _gate_started(loop, grpc_server_port, for_client_gate_port):
    gate_config = chaos.GateRoute(
        name='grpc tcp proxy',
        host_for_client='localhost',
        port_for_client=for_client_gate_port,
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


@pytest.fixture(scope='session')
def grpc_service_port(for_client_gate_port):
    return for_client_gate_port


@pytest.fixture
def extra_client_deps(_gate_started):
    pass


@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate_started):
    _gate_started.to_server_pass()
    _gate_started.to_client_pass()
    _gate_started.start_accepting()
    await _gate_started.sockets_close()  # close keepalive connections

    yield _gate_started
