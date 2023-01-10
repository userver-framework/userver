import socket

import pytest
from pytest_userver import chaos

_ALL_DATA = 512


@pytest.fixture(name='gate', scope='function')
async def _gate(loop, tcp_service_port):
    gate_config = chaos.GateRoute(
        name='tcp proxy',
        host_to_server='localhost',
        port_to_server=tcp_service_port,
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


async def test_chaos_concat_packets(service_client, loop, gate):
    gate.to_client_concat_packets(4)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(gate.get_sockname_for_clients())

    await loop.sock_sendall(sock, b'hi')
    await loop.sock_sendall(sock, b'hi')

    hello = await loop.sock_recv(sock, _ALL_DATA)
    assert hello == b'hellohello'
    assert gate.connections_count() == 1
    gate.to_client_pass()
    sock.close()


async def test_chaos_close_on_data(service_client, loop, gate):
    gate.to_client_close_on_data()

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(gate.get_sockname_for_clients())

    await loop.sock_sendall(sock, b'hi')
    hello = await loop.sock_recv(sock, _ALL_DATA)
    assert not hello
    sock.close()
