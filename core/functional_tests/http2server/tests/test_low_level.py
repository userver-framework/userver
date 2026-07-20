import asyncio
import struct

import h2.connection
import h2.events
import h2.settings
import pytest

import utils

DEFAULT_PATH = '/http2server'

DEFAULT_HEADERS = [
    (':method', 'GET'),
    (':path', f'{DEFAULT_PATH}?type=echo-header'),
    (':scheme', 'http'),
    (':authority', 'localhost'),
    ('echo-header', 'echo'),
]


async def _get_metric(monitor_client, metric_name):
    metric = await monitor_client.single_metric(
        f'server.requests.http2.{metric_name}',
    )
    return metric.value


async def test_http1_broken_bytes(service_client, create_socket):
    async with create_socket() as sock:
        await sock.sendall(b'GET / HTTP/1.1')
        with pytest.raises(asyncio.TimeoutError):
            await sock.recv(1024, timeout=1.0)
        await sock.sendall(b'garbage')
        r = await sock.recv(1024)
        assert len(r.decode('utf-8')) == 0


async def test_settings_and_ping(service_client, create_socket):
    async with create_socket() as sock:
        conn = h2.connection.H2Connection()
        conn.initiate_connection()
        max_streams = 42
        conn.update_settings({
            h2.settings.SettingCodes.MAX_CONCURRENT_STREAMS: max_streams,
        })

        events = []
        while len(events) != utils.EVENTS_COUNT_IN_COMPLETED_STREAM:
            events += await utils.send_and_receive(sock, conn)
        e = events[0]
        assert isinstance(e, h2.events.RemoteSettingsChanged)
        assert utils.MAX_CONCURRENT_STREAMS == e.changed_settings[3].new_value
        assert utils.DEFAULT_FRAME_SIZE == e.changed_settings[5].new_value
        assert isinstance(events[1], h2.events.SettingsAcknowledged)
        assert max_streams == events[1].changed_settings[3].new_value
        assert isinstance(events[2], h2.events.SettingsAcknowledged)

        ping_data = b'12345678'
        conn.ping(ping_data)

        events = []
        while len(events) != 1:
            events += await utils.send_and_receive(sock, conn)
        assert isinstance(events[0], h2.events.PingAckReceived)
        assert ping_data == events[0].ping_data


async def test_invalid_stream(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        invalid_data_frame = utils.create_frame(
            utils.DATA_FRAME,
            utils.EMPTY_FLAGS,
            stream_id=42,
            payload=b'This is some data',
        )
        await sock.sendall(invalid_data_frame)
        receive = await sock.recv(utils.RECEIVE_SIZE)
        events = conn.receive_data(receive)
        assert 1 == len(events)
        assert isinstance(
            events[0],
            h2.events.ConnectionTerminated,
        )  # Is the GOAWAY frame


async def test_h2c_upgrade(create_socket):
    async with create_socket() as sock:
        conn = h2.connection.H2Connection()
        settings_header = conn.initiate_upgrade_connection().decode('ascii')
        request = (
            'GET /http2server?type=echo-header HTTP/1.1\r\n'
            'Host: localhost\r\n'
            'Connection: Upgrade, HTTP2-Settings\r\n'
            'Upgrade: h2c\r\n'
            f'HTTP2-Settings: {settings_header}\r\n'
            'echo-header: upgraded\r\n'
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

        assert b'upgraded' == utils.response_data(events)


async def test_headers_with_continuation_frame(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        stream_id = 1
        header_block = b''.join(utils.encode_header(k, v) for k, v in DEFAULT_HEADERS)
        split_pos = len(header_block) // 2

        # END_STREAM means this GET request has no body; the header block itself
        # continues until END_HEADERS on the CONTINUATION frame.
        headers_frame = utils.create_frame(
            utils.HEADERS_FRAME,
            utils.END_STREAM,
            stream_id,
            header_block[:split_pos],
        )
        continuation_frame = utils.create_frame(
            utils.CONTINUATION_FRAME,
            utils.END_HEADERS,
            stream_id,
            header_block[split_pos:],
        )
        await sock.sendall(headers_frame + continuation_frame)
        receive = await sock.recv(utils.RECEIVE_SIZE)

        _, frame_type, _, response_stream_id, _ = utils.parse_frame_header(receive)
        assert frame_type == utils.HEADERS_FRAME
        assert response_stream_id == stream_id


async def test_non_continuation_frame_during_headers(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        stream_id = 1
        header_block = b''.join(utils.encode_header(k, v) for k, v in DEFAULT_HEADERS)
        headers_frame = utils.create_frame(
            utils.HEADERS_FRAME,
            utils.EMPTY_FLAGS,  # without END_HEADERS!
            stream_id,
            header_block[: len(header_block) // 2],
        )
        # A HEADERS frame without END_HEADERS leaves a header block open. HTTP/2
        # requires the next frame to be CONTINUATION on the same stream.
        data_frame = utils.create_frame(
            utils.DATA_FRAME,
            utils.END_STREAM,
            stream_id,
            b'unexpected data',
        )

        await sock.sendall(headers_frame + data_frame)
        receive = await sock.recv(utils.RECEIVE_SIZE)

        assert utils.GOAWAY_FRAME == receive[utils.FRAME_TYPE_INDEX]
        assert 'unexpected non-CONTINUATION frame or stream_id is invalid' in str(
            receive,
        )


@pytest.mark.skip(reason='TAXICOMMON-10258')
async def test_many_resets(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        for _ in range(1000):
            stream_id = conn.get_next_available_stream_id()
            conn.send_headers(stream_id, DEFAULT_HEADERS, end_stream=True)

            await sock.sendall(conn.data_to_send())

            conn.reset_stream(stream_id, 42)
            await sock.sendall(conn.data_to_send())

        # A simple stream without reset
        stream_id = conn.get_next_available_stream_id()
        conn.send_headers(stream_id, DEFAULT_HEADERS)
        conn.end_stream(stream_id)
        await sock.sendall(conn.data_to_send())

        await utils.receive_simple_response(sock, conn)


def _assert_is_completed_responses(events):
    assert len(events) % utils.EVENTS_COUNT_IN_COMPLETED_STREAM == 0
    for i in range(
        0,
        len(events) - utils.EVENTS_COUNT_IN_COMPLETED_STREAM,
        utils.EVENTS_COUNT_IN_COMPLETED_STREAM,
    ):
        utils.assert_is_completed_response(
            events[i : i + utils.EVENTS_COUNT_IN_COMPLETED_STREAM],
        )


async def test_split_data_frames(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        stream_id = conn.get_next_available_stream_id()
        headers = [
            (':method', 'POST'),
            (':path', f'{DEFAULT_PATH}?type=echo-body'),
            (':scheme', 'http'),
            (':authority', 'localhost'),
        ]
        conn.send_headers(stream_id, headers, end_stream=False)
        conn.send_data(stream_id, b'hello ', end_stream=False)
        conn.send_data(stream_id, b'from ', end_stream=False)
        conn.send_data(stream_id, b'split frames', end_stream=True)
        await sock.sendall(conn.data_to_send())

        events = await utils.receive_until_stream_ended(sock, conn)
        assert b'hello from split frames' == utils.response_data(events)


async def test_empty_data_frame_before_body(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        stream_id = conn.get_next_available_stream_id()
        headers = [
            (':method', 'POST'),
            (':path', f'{DEFAULT_PATH}?type=echo-body'),
            *DEFAULT_HEADERS[2:4],
        ]
        conn.send_headers(stream_id, headers, end_stream=False)
        body = b'body after empty data frame'
        conn.send_data(stream_id, b'', end_stream=False)
        conn.send_data(stream_id, body, end_stream=True)
        await sock.sendall(conn.data_to_send())

        events = await utils.receive_until_stream_ended(sock, conn)
        utils.assert_is_completed_response(events)
        assert events[0].stream_id == stream_id
        assert (b':status', b'200') in events[0].headers
        assert events[1].stream_id == stream_id
        assert events[1].data == body
        assert events[2].stream_id == stream_id


async def test_head_response_has_no_data_frame(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        stream_id = conn.get_next_available_stream_id()
        headers = [
            (':method', 'HEAD'),
            (':path', f'{DEFAULT_PATH}?type=echo-header'),
            (':scheme', 'http'),
            (':authority', 'localhost'),
            ('echo-header', 'body-that-must-not-be-sent'),
        ]
        conn.send_headers(stream_id, headers, end_stream=True)
        await sock.sendall(conn.data_to_send())

        events = await utils.receive_until_stream_ended(sock, conn)
        assert len(events) == 2
        assert isinstance(events[0], h2.events.ResponseReceived)
        assert isinstance(events[1], h2.events.StreamEnded)


async def do_max_streams(sock, conn):
    ids = []
    # create strems in the open state
    for _ in range(utils.MAX_CONCURRENT_STREAMS):
        stream_id = conn.get_next_available_stream_id()
        ids.append(stream_id)
        conn.send_headers(stream_id, DEFAULT_HEADERS)

    assert len(ids) == utils.MAX_CONCURRENT_STREAMS

    # close all streams
    for stream_id in ids:
        conn.end_stream(stream_id)
        await sock.sendall(conn.data_to_send())

    events = []
    expected_frames_count = (
        utils.MAX_CONCURRENT_STREAMS * utils.EVENTS_COUNT_IN_COMPLETED_STREAM
    )  # response =  (ResponseReceived, DataReceived, StreamEnded)
    while len(events) != expected_frames_count:
        receive = await sock.recv(utils.RECEIVE_SIZE)
        if not receive:
            raise RuntimeError('Socket connection was closed by the other side')
        events += conn.receive_data(receive)
    _assert_is_completed_responses(events)


async def test_many_in_flight(
    create_connection,
    monitor_client,
    service_client,
):
    await service_client.update_server_state()

    async with monitor_client.metrics_diff(prefix='server.requests.http2') as differ:
        async with create_connection() as (sock, conn):
            spikes_count = 2
            for _ in range(spikes_count):
                await do_max_streams(sock, conn)

    assert differ.value_at('streams-count') == spikes_count * utils.MAX_CONCURRENT_STREAMS
    assert differ.value_at('streams-close') == spikes_count * utils.MAX_CONCURRENT_STREAMS


async def test_limit_concurrent_streams(
    service_client,
    create_connection,
    monitor_client,
):
    await service_client.update_server_state()

    streams_count = await _get_metric(monitor_client, 'streams-count')
    streams_close = await _get_metric(monitor_client, 'streams-close')

    async with create_connection() as (sock, conn):
        # open the maximum number of streams
        for _ in range(utils.MAX_CONCURRENT_STREAMS):
            stream_id = conn.get_next_available_stream_id()
            conn.send_headers(stream_id, DEFAULT_HEADERS, end_stream=False)
            await sock.sendall(conn.data_to_send())

        await service_client.update_server_state()
        await asyncio.sleep(1.0)

        await service_client.update_server_state()
        assert streams_count + utils.MAX_CONCURRENT_STREAMS == await _get_metric(
            monitor_client,
            'streams-count',
        )
        assert streams_close == await _get_metric(monitor_client, 'streams-close')

        # Go over the limit of strems count. The GOAWAY frame is expected
        stream_id = 203
        payload = b''.join(utils.encode_header(k, v) for k, v in DEFAULT_HEADERS)
        begin_stream_frame = utils.create_frame(
            utils.HEADERS_FRAME,
            utils.END_HEADER_AND_STREAM,
            stream_id,
            payload,
        )

        await sock.sendall(begin_stream_frame)
        receive = await sock.recv(utils.RECEIVE_SIZE)

        assert utils.GOAWAY_FRAME == receive[utils.FRAME_TYPE_INDEX]  # GOAWAY frame
        assert 'request HEADERS: max concurrent streams exceeded' in str(receive)


async def test_request_without_path_resets_stream(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        payload = b''.join(
            utils.encode_header(k, v)
            for k, v in [
                (':method', 'GET'),
                (':scheme', 'http'),
                (':authority', 'localhost'),
            ]
        )
        begin_stream_frame = utils.create_frame(
            utils.HEADERS_FRAME,
            utils.END_HEADER_AND_STREAM,
            stream_id=1,
            payload=payload,
        )

        await sock.sendall(begin_stream_frame)
        receive = await sock.recv(utils.RECEIVE_SIZE)

        payload_size, frame_type, flags, stream_id, payload = utils.parse_frame_header(receive)
        assert payload_size == 4
        assert frame_type == utils.RST_STREAM_FRAME
        assert flags == utils.EMPTY_FLAGS
        assert stream_id == 1
        assert int.from_bytes(payload, byteorder='big') == utils.PROTOCOL_ERROR_CODE

        valid_stream_frame = utils.create_frame(
            utils.HEADERS_FRAME,
            utils.END_HEADER_AND_STREAM,
            stream_id=3,
            payload=b''.join(utils.encode_header(k, v) for k, v in DEFAULT_HEADERS),
        )
        await sock.sendall(valid_stream_frame)
        receive = await sock.recv(utils.RECEIVE_SIZE)

        _, frame_type, _, stream_id, _ = utils.parse_frame_header(receive)
        assert frame_type == utils.HEADERS_FRAME
        assert stream_id == 3


async def test_single_reset_keeps_connection_usable(
    create_connection,
    monitor_client,
    service_client,
):
    await service_client.update_server_state()

    async with monitor_client.metrics_diff(prefix='server.requests.http2') as differ:
        async with create_connection() as (sock, conn):
            stream_id = conn.get_next_available_stream_id()
            conn.send_headers(stream_id, DEFAULT_HEADERS, end_stream=False)
            await sock.sendall(conn.data_to_send())

            conn.reset_stream(stream_id)
            await sock.sendall(conn.data_to_send())

            stream_id = conn.get_next_available_stream_id()
            conn.send_headers(stream_id, DEFAULT_HEADERS, end_stream=True)
            await sock.sendall(conn.data_to_send())
            await utils.receive_simple_response(sock, conn)

    assert differ.value_at('reset-streams') == 1


async def test_stream_already_closed(create_connection, service_client):
    async with create_connection() as (sock, conn):

        async def open_and_close_simple_stream():
            stream_id = conn.get_next_available_stream_id()
            conn.send_headers(stream_id, DEFAULT_HEADERS)
            conn.end_stream(stream_id)
            await utils.receive_simple_response(sock, conn)
            return stream_id

        stream_id = await open_and_close_simple_stream()
        assert stream_id == 1

        reset_closed_stream = utils.create_frame(
            utils.RST_STREAM_FRAME,
            utils.EMPTY_FLAGS,
            stream_id,
            struct.pack('>I', utils.PROTOCOL_ERROR_CODE),
        )

        # RST_STREAM for an already closed stream should not poison the connection.
        await sock.sendall(reset_closed_stream)
        await sock.sendall(reset_closed_stream)

        stream_id = await open_and_close_simple_stream()
        assert stream_id == 3


async def test_streams_with_the_same_id(create_connection, service_client):
    async with create_connection() as (sock, conn):
        stream_id = 1
        payload = b''.join(utils.encode_header(k, v) for k, v in DEFAULT_HEADERS)
        begin_stream_frame = utils.create_frame(
            utils.HEADERS_FRAME,
            utils.EMPTY_FLAGS,
            stream_id,
            payload,
        )
        await sock.sendall(begin_stream_frame)
        await sock.sendall(begin_stream_frame)
        receive = await sock.recv(utils.RECEIVE_SIZE)

        assert utils.GOAWAY_FRAME == receive[utils.FRAME_TYPE_INDEX]  # GOAWAY frame
        assert 'unexpected non-CONTINUATION frame or stream_id is invalid' in str(
            receive,
        )


# RFC 9113 8.3 does not fix an order among the pseudo-header fields, so a
# request must be routed identically regardless of it. h2load, for example,
# serializes :path first and :method after :scheme and :authority.
PSEUDO_HEADER_ORDERS = [
    pytest.param(order, id=order_id)
    for order_id, order in [
        ('method-first', [':method', ':path', ':scheme', ':authority']),
        ('h2load', [':path', ':scheme', ':authority', ':method']),
        ('method-last', [':path', ':authority', ':scheme', ':method']),
        ('path-last', [':authority', ':scheme', ':method', ':path']),
    ]
]


def _make_headers(order, query, extra):
    values = {
        ':method': 'GET',
        ':path': f'{DEFAULT_PATH}?{query}',
        ':scheme': 'http',
        ':authority': 'localhost',
    }
    return [(name, values[name]) for name in order] + extra


@pytest.mark.parametrize('order', PSEUDO_HEADER_ORDERS)
async def test_pseudo_header_order_is_irrelevant(
    create_connection,
    service_client,
    order,
):
    headers = _make_headers(order, 'type=echo-header', [('echo-header', 'reordered')])
    async with create_connection() as (sock, conn):
        stream_id = conn.get_next_available_stream_id()
        conn.send_headers(stream_id, headers, end_stream=True)
        await sock.sendall(conn.data_to_send())

        events = await utils.receive_until_stream_ended(sock, conn)
        response = next(event for event in events if isinstance(event, h2.events.ResponseReceived))
        assert b'200' == dict(response.headers)[b':status']
        assert b'reordered' == utils.response_data(events)


@pytest.mark.parametrize('order', PSEUDO_HEADER_ORDERS)
async def test_pseudo_header_order_with_request_body(
    create_connection,
    service_client,
    order,
):
    # Handler matching installs the per-handler request limits, which must be
    # in place before the DATA frames arrive - so it must happen at the end
    # of the header block whatever the pseudo-header order was.
    body = b'0123456789abcdef' * 64
    headers = _make_headers(order, 'type=echo-body', [])
    headers = [(':method', 'POST') if name == ':method' else (name, value) for name, value in headers]
    async with create_connection() as (sock, conn):
        stream_id = conn.get_next_available_stream_id()
        conn.send_headers(stream_id, headers, end_stream=False)
        conn.send_data(stream_id, body, end_stream=True)
        await sock.sendall(conn.data_to_send())

        events = await utils.receive_until_stream_ended(sock, conn)
        response = next(event for event in events if isinstance(event, h2.events.ResponseReceived))
        assert b'200' == dict(response.headers)[b':status']
        assert body == utils.response_data(events)
