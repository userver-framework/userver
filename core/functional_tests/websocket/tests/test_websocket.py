import asyncio
import logging

import pytest
import websockets

# Disabling redundant logs from third party library
logger = logging.getLogger('websockets.client')
logger.setLevel('INFO')


async def test_echo(websocket_client):
    async with websocket_client.get('chat') as chat:
        await chat.send('hello')
        response = await chat.recv()
        assert response == 'hello'


async def test_echo_with_continuation(websocket_client):
    async with websocket_client.get('chat') as chat:
        # Send first fragment (not final, text frame)
        await chat.write_frame(fin=False, opcode=0x1, data=b'First')
        # Send intermediate fragment (not final, continuation frame)
        await chat.write_frame(fin=False, opcode=0x0, data=b' second')
        # Send last fragment (final, continuation frame)
        await chat.write_frame(fin=True, opcode=0x0, data=b' third')

        response = await chat.recv()
        assert response == 'First second third'


async def test_close_by_server(websocket_client):
    async with websocket_client.get('chat') as chat:
        await chat.send('close')
        with pytest.raises(websockets.exceptions.ConnectionClosedOK) as exc:
            await chat.recv()
        assert exc.value.rcvd.code == 1001


async def test_ping(websocket_client):
    async with websocket_client.get('chat') as chat:
        await chat.ping(data='ping')

        # extra msg to be sure ping is processed
        await chat.send('hello')
        response = await chat.recv()
        assert response == 'hello'


async def test_big_16(websocket_client):
    async with websocket_client.get('chat') as chat:
        msg = 'a' * 200
        await chat.send(msg)
        response = await chat.recv()
        assert response == msg


async def test_big_32(websocket_client):
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
        with pytest.raises(websockets.exceptions.ConnectionClosed) as exc:
            await chat.send(msg)
            await chat.recv()
        assert exc.value.rcvd.code == 1009


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


async def test_duplex_but_handler_alt(websocket_client):
    async with websocket_client.get('handler-alt') as chat:
        await chat.send('ping')
        response = await chat.recv()
        assert response == 'ping'


async def test_two_but_handler_alt(websocket_client):
    async with websocket_client.get('handler-alt') as chat1:
        async with websocket_client.get('handler-alt') as chat2:
            for _ in range(10):
                await chat1.send('A')
            for _ in range(10):
                await chat2.send('B')

            for _ in range(10):
                msg = await chat2.recv()
                assert msg == 'B'
            for _ in range(10):
                msg = await chat1.recv()
                assert msg == 'A'


async def test_ping_pong(websocket_client):
    async with websocket_client.get('ping-pong'):
        await asyncio.sleep(1)


async def test_ping_pong_close(websocket_client):
    async with websocket_client.get('ping-pong') as chat:
        websocket_client.ping_interval = None
        websocket_client.ping_timeout = None

        for _ in range(20):
            try:
                await chat.recv()
                await asyncio.sleep(1)
            except websockets.exceptions.ConnectionClosed:
                connection_closed_by_ping = True
                break

        assert connection_closed_by_ping


async def test_upgrade_header_with_tab_then_reconnect(service_port):
    reader, writer = await asyncio.open_connection('localhost', service_port)

    # request with tab in Upgrade header
    bad_request = (
        'GET /chat HTTP/1.1\r\n'
        'Sec-WebSocket-Version: 13\r\n'
        'Sec-WebSocket-Key: fQU/VaAZ3+lpmSjWKevurQ==\r\n'
        'Connection: Upgrade\r\n'
        'Upgrade:\twebsocket\r\n'
        '\r\n'
    )

    writer.write(bad_request.encode('ascii'))
    await writer.drain()
    writer.close()
    await writer.wait_closed()

    # 2. Check new connection can be established
    async with websockets.connect(f'ws://localhost:{service_port}/chat') as chat:
        await chat.send('ping')
        resp = await chat.recv()
        assert resp == 'ping'
