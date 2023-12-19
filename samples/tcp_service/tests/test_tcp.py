# /// [Functional test]
import socket

import pytest


async def test_basic(service_client, loop, tcp_service_port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', tcp_service_port))

    await loop.sock_sendall(sock, b'hi')
    hello = await loop.sock_recv(sock, 5)
    assert hello == b'hello'

    await loop.sock_sendall(sock, b'whats up?')
    with pytest.raises(ConnectionResetError):
        await loop.sock_recv(sock, 1)
    # /// [Functional test]
