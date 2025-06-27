import asyncio
import socket


async def test_send(service_binary):
    with socket.socket(socket.AF_INET6, socket.SOCK_STREAM) as server:
        server.bind(('::', 0))
        server.listen()
        server.setblocking(False)

        subprocess = await asyncio.create_subprocess_exec(
            service_binary, '--port', str(server.getsockname()[1]), stdin=asyncio.subprocess.PIPE
        )

        subprocess.stdin.write(b'PING\n')
        loop = asyncio.get_event_loop()
        client, _ = await loop.sock_accept(server)

        request = await loop.sock_recv(client, 4)
        assert request == b'PING'

        subprocess.terminate()
        await subprocess.wait()
        client.close()


async def test_listen(service_binary):
    subprocess = await asyncio.create_subprocess_exec(service_binary, '-l', stdout=asyncio.subprocess.PIPE)
    await asyncio.sleep(0.3)  # give time to open listening socket

    with socket.socket(socket.AF_INET6, socket.SOCK_STREAM) as client:
        client.connect(('localhost', 3333))
        client.setblocking(False)

        loop = asyncio.get_event_loop()
        await loop.sock_sendall(client, b'PING')

        response = await subprocess.stdout.read(4)
        assert response == b'PING'

        subprocess.terminate()
        await subprocess.wait()
