# pylint: disable=protected-access
import pytest

from pytest_userver import chaos


@pytest.fixture(name='for_client_gate_port', scope='session')
def _for_client_gate_port(request):
    # This fixture might be defined in an outer scope.
    if 'for_client_gate_port_override' in request.fixturenames:
        return request.getfixturevalue('for_client_gate_port_override')
    return 11433


@pytest.fixture()
def modified_service_client(
        service_client, service_port, for_client_gate_port,
):
    client = service_client._client
    client._base_url = client._base_url.replace(
        str(service_port), str(for_client_gate_port),
    )
    return service_client


@pytest.fixture()
async def _gate_started(loop, for_client_gate_port, service_port):
    gate_config = chaos.GateRoute(
        name='tcp proxy',
        host_for_client='localhost',
        port_for_client=for_client_gate_port,
        host_to_server='localhost',
        port_to_server=service_port,
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture
def client_deps(_gate_started):
    pass


@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate_started):
    _gate_started.to_server_pass()
    _gate_started.to_client_pass()
    _gate_started.start_accepting()
    await _gate_started.sockets_close()  # close keepalive connections

    yield _gate_started
