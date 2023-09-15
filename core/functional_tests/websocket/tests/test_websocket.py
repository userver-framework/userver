import websockets


async def test_echo(websocket_client):
    async with websocket_client.get('chat') as chat:
        await chat.send('hello')
        response = await chat.recv()
        assert response == 'hello'


async def test_close_by_server(websocket_client):
    async with websocket_client.get('chat') as chat:
        await chat.send('close')
        try:
            await chat.recv()
            assert False
        except websockets.exceptions.ConnectionClosedOK as e:
            assert e.rcvd.code == 1001


async def test_ping(websocket_client):
    async with websocket_client.get('chat') as chat:
        await chat.ping(data='ping')

        # extra msg to be sure ping is processed
        await chat.send('hello')
        response = await chat.recv()
        assert response == 'hello'


async def test_big(websocket_client):
    async with websocket_client.get('chat') as chat:
        msg = 'hello' * 10000
        await chat.send(msg)
        response = await chat.recv()
        assert response == msg


async def test_cycle(websocket_client):
    async with websocket_client.get('chat') as chat:
        for i in range(1000):
            msg = str(i)
            await chat.send(msg)
            response = await chat.recv()
            assert response == msg


async def test_too_big(websocket_client):
    async with websocket_client.get('chat') as chat:
        msg = 'hello' * 100000
        try:
            await chat.send(msg)
            await chat.recv()
            assert False
        except websockets.exceptions.ConnectionClosed as e:
            assert e.rcvd.code == 1009


async def test_origin(service_client, service_port):
    async with websockets.connect(
            f'ws://localhost:{service_port}/chat',
            extra_headers={'Origin': 'localhost'},
    ) as chat:
        response = await chat.recv()
        assert response == 'localhost'


async def test_duplex(websocket_client):
    async with websocket_client.get('duplex') as chat:
        await chat.send('ping')
        response = await chat.recv()
        assert response == b'ping'


async def test_two(websocket_client):
    async with websocket_client.get('duplex') as chat1:
        async with websocket_client.get('duplex') as chat2:
            for _ in range(10):
                await chat1.send('A')
            for _ in range(10):
                await chat2.send('B')

            for _ in range(10):
                msg = await chat2.recv()
                assert msg == b'B'
            for _ in range(10):
                msg = await chat1.recv()
                assert msg == b'A'
