import asyncio

import h2.connection
import h2.events
import h2.settings

import utils

DEFAULT_PATH = '/http2server-stream'


def _stream_headers(query: str) -> list:
    return [
        (':method', 'GET'),
        (':path', f'{DEFAULT_PATH}?{query}'),
        (':scheme', 'http'),
        (':authority', 'localhost'),
    ]


async def test_body_stream(http2_client, service_client, dynamic_config):
    part = 'part'
    count = 100
    r = await http2_client.get(
        DEFAULT_PATH,
        params={'type': 'eq', 'body_part': part, 'count': count},
    )
    assert 200 == r.status_code
    assert part * count == r.text


async def _stream_request(client, req_per_client):
    for _ in range(req_per_client):
        data = 'abcdefgh' * 2**7  # size is 1024
        r = await client.get(DEFAULT_PATH, params={'type': 'ne'}, data=data)
        assert 200 == r.status_code
        assert data == r.text


async def test_body_stream_small_pieces(
    http2_client,
    service_client,
    dynamic_config,
):
    await _stream_request(http2_client, 1)


async def test_body_stream_concurrent(
    http2_client,
    service_client,
    dynamic_config,
):
    clients_count = 2
    req_per_client = 10
    tasks = [_stream_request(http2_client, req_per_client) for _ in range(clients_count)]
    await asyncio.gather(*tasks)


async def test_body_stream_no_head_of_line_blocking(
    http2_client,
    service_client,
    dynamic_config,
):
    # A slow streamed response (~5s) on one stream must not delay other
    # requests multiplexed on the same connection. If it did, each "fast"
    # request below would complete only after the slow stream finishes and
    # trip its timeout.
    part = 'x'
    count = 50
    slow = asyncio.create_task(
        http2_client.get(
            DEFAULT_PATH,
            params={
                'type': 'eq',
                'body_part': part,
                'count': count,
                'delay_ms': 100,
            },
            timeout=30.0,
        ),
    )
    try:
        for _ in range(5):
            r = await asyncio.wait_for(
                http2_client.get(
                    '/http2server',
                    params={'type': 'echo-body'},
                    data='ping',
                ),
                timeout=2.0,
            )
            assert 200 == r.status_code
            assert 'ping' == r.text
    finally:
        r = await slow
    assert 200 == r.status_code
    assert part * count == r.text


async def test_reset_mid_stream_keeps_connection_usable(
    create_connection,
    service_client,
):
    async with create_connection() as (sock, conn):
        # A slow stream: the handler will keep producing for ~3s after the
        # client resets the stream; those events must be dropped, not tear
        # down the connection or the process.
        stream_id = conn.get_next_available_stream_id()
        conn.send_headers(
            stream_id,
            _stream_headers('type=eq&body_part=part&count=30&delay_ms=100'),
            end_stream=True,
        )
        await sock.sendall(conn.data_to_send())

        events = []
        while not any(isinstance(event, h2.events.DataReceived) for event in events):
            events += await utils.send_and_receive(sock, conn)

        conn.reset_stream(stream_id, error_code=0x8)  # CANCEL
        await sock.sendall(conn.data_to_send())

        # The same connection must still serve requests, concurrently with
        # the handler of the reset stream still pushing body parts.
        echo_stream_id = conn.get_next_available_stream_id()
        conn.send_headers(
            echo_stream_id,
            [
                (':method', 'GET'),
                (':path', '/http2server?type=echo-header'),
                (':scheme', 'http'),
                (':authority', 'localhost'),
                ('echo-header', 'still-alive'),
            ],
            end_stream=True,
        )
        await sock.sendall(conn.data_to_send())

        events = await utils.receive_until_stream_ended(sock, conn)
        assert b'still-alive' == utils.response_data(events)


async def test_h2c_upgrade_with_streamed_response(create_socket, service_client):
    # The first request of an h2c upgrade is parsed as HTTP/1.1, so the
    # streamed response has no HTTP/2 producer; it must degrade to a
    # buffered send instead of hanging on a forever-deferred provider.
    async with create_socket() as sock:
        conn = h2.connection.H2Connection()
        settings_header = conn.initiate_upgrade_connection().decode('ascii')
        request = (
            f'GET {DEFAULT_PATH}?type=eq&body_part=part&count=10 HTTP/1.1\r\n'
            'Host: localhost\r\n'
            'Connection: Upgrade, HTTP2-Settings\r\n'
            'Upgrade: h2c\r\n'
            f'HTTP2-Settings: {settings_header}\r\n'
            '\r\n'
        )
        await sock.sendall(request.encode('ascii'))

        receive = b''
        while utils.HTTP1_HEADERS_END not in receive:
            receive += await sock.recv(utils.RECEIVE_SIZE)
        headers, _, http2_data = receive.partition(utils.HTTP1_HEADERS_END)
        assert headers.startswith(b'HTTP/1.1 101 Switching Protocols')

        events = conn.receive_data(http2_data) if http2_data else []
        await sock.sendall(conn.data_to_send())
        while not any(isinstance(event, h2.events.StreamEnded) for event in events):
            events += await utils.send_and_receive(sock, conn)

        assert b'part' * 10 == utils.response_data(events)


async def test_flow_control_backpressure(create_connection, service_client):
    # With a tiny stream window the server may only produce as fast as the
    # client opens the window with WINDOW_UPDATEs; the deferred provider must
    # resume each time instead of stalling or flooding.
    window = 1024
    part = 'x' * 1024
    count = 100  # total body is 100 KiB, also exceeds the connection window

    async with create_connection() as (sock, conn):
        conn.update_settings(
            {h2.settings.SettingCodes.INITIAL_WINDOW_SIZE: window},
        )
        stream_id = conn.get_next_available_stream_id()
        conn.send_headers(
            stream_id,
            _stream_headers(f'type=eq&body_part={part}&count={count}&delay_ms=0'),
            end_stream=True,
        )
        await sock.sendall(conn.data_to_send())

        body = b''
        ended = False
        while not ended:
            receive = await sock.recv(utils.RECEIVE_SIZE)
            if not receive:
                raise RuntimeError('Socket connection was closed by the other side')
            for event in conn.receive_data(receive):
                if isinstance(event, h2.events.DataReceived):
                    body += event.data
                    conn.acknowledge_received_data(
                        event.flow_controlled_length,
                        event.stream_id,
                    )
                elif isinstance(event, h2.events.StreamEnded):
                    ended = True
            data = conn.data_to_send()
            if data:
                await sock.sendall(data)

        assert len(body) == len(part) * count
        assert part.encode() * count == body
