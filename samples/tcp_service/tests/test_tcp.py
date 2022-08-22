# /// [Functional test]
import socket


async def test_basic(service_client, loop):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('localhost', 8180))

    await loop.sock_sendall(s, b'hi')
    hello = await loop.sock_recv(s, 5)
    assert hello == b'hello'

    await loop.sock_sendall(s, b'whats up?')
    try:
        await loop.sock_recv(s, 1)
        assert False
    except ConnectionResetError:
        pass
    # /// [Functional test]
