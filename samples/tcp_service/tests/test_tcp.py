# /// [Functional test]
import pytest


async def test_basic(service_client, asyncio_socket, tcp_service_port):
    sock = asyncio_socket.tcp()
    await sock.connect(('localhost', tcp_service_port))

    await sock.sendall(b'hi')
    hello = await sock.recv(5)
    assert hello == b'hello'

    await sock.sendall(b'whats up?')
    with pytest.raises(ConnectionResetError):
        await sock.recv(1)
    # /// [Functional test]
