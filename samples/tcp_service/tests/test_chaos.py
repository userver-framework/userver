import pytest
from pytest_userver import chaos

_ALL_DATA = 512


@pytest.fixture(name='gate', scope='function')
async def _gate(tcp_service_port):
    gate_config = chaos.GateRoute(
        name='tcp proxy',
        host_to_server='localhost',
        port_to_server=tcp_service_port,
    )
    async with chaos.TcpGate(gate_config) as proxy:
        yield proxy


async def test_chaos_concat_packets(asyncio_socket, service_client, gate):
    await gate.to_client_concat_packets(10)

    sock = asyncio_socket.tcp()
    await sock.connect(gate.get_sockname_for_clients())

    await sock.sendall(b'hi')
    await sock.sendall(b'hi')

    hello = await sock.recv(_ALL_DATA)
    assert hello == b'hellohello'
    assert gate.connections_count() == 1
    await gate.to_client_pass()
    sock.close()


async def test_chaos_close_on_data(service_client, asyncio_socket, gate):
    await gate.to_client_close_on_data()

    sock = asyncio_socket.tcp()
    await sock.connect(gate.get_sockname_for_clients())

    await sock.sendall(b'hi')
    hello = await sock.recv(_ALL_DATA)
    assert not hello
    sock.close()
