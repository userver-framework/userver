import asyncio
import socket


async def test_send(service_binary):
    with socket.socket(socket.AF_INET6, socket.SOCK_STREAM) as server:
        server.bind(('::', 0))
        server.listen()
        server.setblocking(False)

        subprocess = await asyncio.create_subprocess_exec(
            service_binary,
            '--port',
            str(server.getsockname()[1]),
            stdin=asyncio.subprocess.PIPE,
        )

        subprocess.stdin.write(b'PING\n')
        loop = asyncio.get_event_loop()
        client, _ = await loop.sock_accept(server)

        request = await loop.sock_recv(client, 4)
        assert request == b'PING'

        subprocess.terminate()
        await subprocess.wait()
        client.close()


def _get_free_port() -> int:
    with socket.socket(socket.AF_INET6, socket.SOCK_STREAM) as sock:
        sock.bind(('::', 0))
        return sock.getsockname()[1]


async def test_listen(service_binary):
    port = _get_free_port()

    subprocess = await asyncio.create_subprocess_exec(
        service_binary,
        '-l',
        '--port',
        str(port),
        stdout=asyncio.subprocess.PIPE,
    )

    # try several times to make sure netcat has opened the tcp port
    for i in range(1, 100):
        try:
            client = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
            client.connect(('localhost', port))
            break
        except ConnectionRefusedError:
            if client:
                client.close()
                client = None
            await asyncio.sleep(0.3)
            continue
    else:
        assert False, 'failed to connect to netcat'

    try:
        client.setblocking(False)

        loop = asyncio.get_event_loop()
        await loop.sock_sendall(client, b'PING')

        response = await subprocess.stdout.read(4)
        assert response == b'PING'

        subprocess.terminate()
        await subprocess.wait()
    finally:
        client.close()
