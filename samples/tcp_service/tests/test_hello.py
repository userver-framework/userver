# /// [Functional test]
import socket


async def test_ping(service_client):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('localhost', 8180))
    s.sendall(b'hi')
    hello = s.recv(5)
    assert hello == b'hello'
    # /// [Functional test]
