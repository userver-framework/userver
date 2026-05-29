import asyncio
import contextlib
import logging
import socket
import struct

import h2.connection
import h2.events
import h2.settings
import pytest

DEFAULT_PATH = '/http2server'
DEFAULT_DATA = {'hello': 'world'}
DEFAULT_FRAME_SIZE = 1 << 14
RECEIVE_SIZE = 1 << 26
MAX_CONCURRENT_STREAMS = 100
HTTP1_HEADERS_END = b'\r\n\r\n'

DATA_FRAME = 0x0
HEADERS_FRAME = 0x01
RST_STREAM_FRAME = 0x03
GOAWAY_FRAME = 0x07
CONTINUATION_FRAME = 0x09
EMPTY_FLAGS = 0x0
END_STREAM = 0x01
END_HEADERS = 0x04
END_HEADER_AND_STREAM = 0x05
PROTOCOL_ERROR_CODE = 0x01

FRAME_TYPE_INDEX = 3

DEFAULT_HEADERS = [
    (':method', 'GET'),
    (':path', f'{DEFAULT_PATH}?type=echo-header'),
    (':scheme', 'http'),
    (':authority', 'localhost'),
    ('echo-header', 'echo'),
]

EVENTS_COUNT_IN_COMPLETED_STREAM = 3


async def _get_metric(monitor_client, metric_name):
    metric = await monitor_client.single_metric(
        f'server.requests.http2.{metric_name}',
    )
    return metric.value


@pytest.fixture(name='create_socket')
async def _create_socket(service_port, asyncio_socket):
    @contextlib.asynccontextmanager
    async def create_socket():
        logging.debug('Connecting to localhost:%d', service_port)
        try:
            sock = asyncio_socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            await sock.connect(('localhost', service_port))
            logging.debug('Connected: %r', sock)

            yield sock

        finally:
            sock.close()

    return create_socket


async def test_http1_broken_bytes(service_client, create_socket):
    async with create_socket() as sock:
        await sock.sendall(b'GET / HTTP/1.1')
        with pytest.raises(asyncio.TimeoutError):
            await sock.recv(1024, timeout=1.0)
        await sock.sendall(b'garbage')
        r = await sock.recv(1024)
        assert len(r.decode('utf-8')) == 0


async def _send_and_receive(sock, conn):
    await sock.sendall(conn.data_to_send())
    receive = await sock.recv(RECEIVE_SIZE)
    return conn.receive_data(receive)


async def test_settings_and_ping(service_client, create_socket):
    async with create_socket() as sock:
        conn = h2.connection.H2Connection()
        conn.initiate_connection()
        max_streams = 42
        conn.update_settings({
            h2.settings.SettingCodes.MAX_CONCURRENT_STREAMS: max_streams,
        })

        events = []
        while len(events) != EVENTS_COUNT_IN_COMPLETED_STREAM:
            events += await _send_and_receive(sock, conn)
        e = events[0]
        assert isinstance(e, h2.events.RemoteSettingsChanged)
        assert MAX_CONCURRENT_STREAMS == e.changed_settings[3].new_value
        assert DEFAULT_FRAME_SIZE == e.changed_settings[5].new_value
        assert isinstance(events[1], h2.events.SettingsAcknowledged)
        assert max_streams == events[1].changed_settings[3].new_value
        assert isinstance(events[2], h2.events.SettingsAcknowledged)

        ping_data = b'12345678'
        conn.ping(ping_data)

        events = []
        while len(events) != 2:
            events += await _send_and_receive(sock, conn)
        assert isinstance(events[0], h2.events.PingAckReceived)
        assert ping_data == events[0].ping_data
        assert isinstance(events[1], h2.events.PingReceived)
        assert b'\x00\x00\x00\x00\x00\x00\x00\x00' == events[1].ping_data


@pytest.fixture(name='create_connection')
async def _create_connection(create_socket):
    @contextlib.asynccontextmanager
    async def create_connection():
        conn = h2.connection.H2Connection()
        conn.initiate_connection()

        async with create_socket() as sock:
            events = []
            while len(events) != 2:
                events += await _send_and_receive(sock, conn)
            assert isinstance(events[0], h2.events.RemoteSettingsChanged)
            assert MAX_CONCURRENT_STREAMS == events[0].changed_settings[3].new_value
            assert DEFAULT_FRAME_SIZE == events[0].changed_settings[5].new_value
            assert isinstance(events[1], h2.events.SettingsAcknowledged)

            logging.debug('Connection successfully created')

            yield sock, conn

    return create_connection


def _create_frame(frame_type, flags, stream_id, payload):
    header = (
        struct.pack('>I', len(payload))[1:]  # length 3 bytes
        + struct.pack('B', frame_type)  # type 1 byte
        + struct.pack('B', flags)  # flags 1 byte
        + struct.pack('>I', stream_id & 0x7FFFFFFF)  # stream_id 4 bytes
    )
    assert len(header) == 9
    return header + payload


def _parse_frame_header(frame):
    payload_size = int.from_bytes(frame[:3], byteorder='big')
    stream_id = int.from_bytes(frame[5:9], byteorder='big') & 0x7FFFFFFF
    payload = frame[9 : 9 + payload_size]
    return payload_size, frame[3], frame[4], stream_id, payload


def _assert_is_completed_response(events):
    assert len(events) == EVENTS_COUNT_IN_COMPLETED_STREAM
    assert isinstance(events[0], h2.events.ResponseReceived)
    assert isinstance(events[1], h2.events.DataReceived)
    assert isinstance(events[2], h2.events.StreamEnded)


async def _receive_simple_response(sock, conn):
    events = []
    while len(events) != EVENTS_COUNT_IN_COMPLETED_STREAM:
        events += await _send_and_receive(sock, conn)
    _assert_is_completed_response(events)


async def _receive_until_stream_ended(sock, conn, events=None):
    events = list(events or [])
    while not any(isinstance(event, h2.events.StreamEnded) for event in events):
        receive = await sock.recv(RECEIVE_SIZE)
        if not receive:
            raise RuntimeError('Socket connection was closed by the other side')
        events += conn.receive_data(receive)
    return events


def _response_data(events):
    return b''.join(event.data for event in events if isinstance(event, h2.events.DataReceived))


async def test_invalid_stream(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        invalid_data_frame = _create_frame(
            DATA_FRAME,
            EMPTY_FLAGS,
            stream_id=42,
            payload=b'This is some data',
        )
        await sock.sendall(invalid_data_frame)
        receive = await sock.recv(RECEIVE_SIZE)
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
        while HTTP1_HEADERS_END not in receive:
            receive += await sock.recv(RECEIVE_SIZE)
        headers, _, http2_data = receive.partition(HTTP1_HEADERS_END)
        assert headers.startswith(b'HTTP/1.1 101 Switching Protocols')

        events = conn.receive_data(http2_data) if http2_data else []
        await sock.sendall(conn.data_to_send())
        events = await _receive_until_stream_ended(sock, conn, events)

        assert b'upgraded' == _response_data(events)


async def test_headers_with_continuation_frame(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        stream_id = 1
        header_block = b''.join(_encode_header(k, v) for k, v in DEFAULT_HEADERS)
        split_pos = len(header_block) // 2

        # END_STREAM means this GET request has no body; the header block itself
        # continues until END_HEADERS on the CONTINUATION frame.
        headers_frame = _create_frame(
            HEADERS_FRAME,
            END_STREAM,
            stream_id,
            header_block[:split_pos],
        )
        continuation_frame = _create_frame(
            CONTINUATION_FRAME,
            END_HEADERS,
            stream_id,
            header_block[split_pos:],
        )
        await sock.sendall(headers_frame + continuation_frame)
        receive = await sock.recv(RECEIVE_SIZE)

        _, frame_type, _, response_stream_id, _ = _parse_frame_header(receive)
        assert frame_type == HEADERS_FRAME
        assert response_stream_id == stream_id


async def test_non_continuation_frame_during_headers(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        stream_id = 1
        header_block = b''.join(_encode_header(k, v) for k, v in DEFAULT_HEADERS)
        headers_frame = _create_frame(
            HEADERS_FRAME,
            EMPTY_FLAGS,  # without END_HEADERS!
            stream_id,
            header_block[: len(header_block) // 2],
        )
        # A HEADERS frame without END_HEADERS leaves a header block open. HTTP/2
        # requires the next frame to be CONTINUATION on the same stream.
        data_frame = _create_frame(
            DATA_FRAME,
            END_STREAM,
            stream_id,
            b'unexpected data',
        )

        await sock.sendall(headers_frame + data_frame)
        receive = await sock.recv(RECEIVE_SIZE)

        assert GOAWAY_FRAME == receive[FRAME_TYPE_INDEX]
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

        await _receive_simple_response(sock, conn)


def _encode_header(name, value):
    name_encoded = name.encode('utf-8')
    value_encoded = value.encode('utf-8')
    zero = struct.pack('>B', 0x0)
    return (
        zero + struct.pack('B', len(name_encoded)) + name_encoded + struct.pack('B', len(value_encoded)) + value_encoded
    )


def _assert_is_completed_responses(events):
    assert len(events) % EVENTS_COUNT_IN_COMPLETED_STREAM == 0
    for i in range(0, len(events) - EVENTS_COUNT_IN_COMPLETED_STREAM, EVENTS_COUNT_IN_COMPLETED_STREAM):
        _assert_is_completed_response(events[i : i + EVENTS_COUNT_IN_COMPLETED_STREAM])


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

        events = await _receive_until_stream_ended(sock, conn)
        assert b'hello from split frames' == _response_data(events)


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

        events = await _receive_until_stream_ended(sock, conn)
        _assert_is_completed_response(events)
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

        events = await _receive_until_stream_ended(sock, conn)
        assert len(events) == 2
        assert isinstance(events[0], h2.events.ResponseReceived)
        assert isinstance(events[1], h2.events.StreamEnded)


async def do_max_streams(sock, conn):
    ids = []
    # create strems in the open state
    for _ in range(MAX_CONCURRENT_STREAMS):
        stream_id = conn.get_next_available_stream_id()
        ids.append(stream_id)
        conn.send_headers(stream_id, DEFAULT_HEADERS)

    assert len(ids) == MAX_CONCURRENT_STREAMS

    # close all streams
    for stream_id in ids:
        conn.end_stream(stream_id)
        await sock.sendall(conn.data_to_send())

    events = []
    expected_frames_count = (
        MAX_CONCURRENT_STREAMS * EVENTS_COUNT_IN_COMPLETED_STREAM
    )  # response =  (ResponseReceived, DataReceived, StreamEnded)
    while len(events) != expected_frames_count:
        receive = await sock.recv(RECEIVE_SIZE)
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

    assert differ.value_at('streams-count') == spikes_count * MAX_CONCURRENT_STREAMS
    assert differ.value_at('streams-close') == spikes_count * MAX_CONCURRENT_STREAMS


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
        for _ in range(MAX_CONCURRENT_STREAMS):
            stream_id = conn.get_next_available_stream_id()
            conn.send_headers(stream_id, DEFAULT_HEADERS, end_stream=False)
            await sock.sendall(conn.data_to_send())

        await service_client.update_server_state()
        await asyncio.sleep(1.0)

        await service_client.update_server_state()
        assert streams_count + MAX_CONCURRENT_STREAMS == await _get_metric(
            monitor_client,
            'streams-count',
        )
        assert streams_close == await _get_metric(monitor_client, 'streams-close')

        # Go over the limit of strems count. The GOAWAY frame is expected
        stream_id = 203
        payload = b''.join(_encode_header(k, v) for k, v in DEFAULT_HEADERS)
        begin_stream_frame = _create_frame(
            HEADERS_FRAME,
            END_HEADER_AND_STREAM,
            stream_id,
            payload,
        )

        await sock.sendall(begin_stream_frame)
        receive = await sock.recv(RECEIVE_SIZE)

        assert GOAWAY_FRAME == receive[FRAME_TYPE_INDEX]  # GOAWAY frame
        assert 'request HEADERS: max concurrent streams exceeded' in str(receive)


async def test_request_without_path_resets_stream(create_connection, service_client):
    await service_client.update_server_state()
    async with create_connection() as (sock, conn):
        payload = b''.join(
            _encode_header(k, v)
            for k, v in [
                (':method', 'GET'),
                (':scheme', 'http'),
                (':authority', 'localhost'),
            ]
        )
        begin_stream_frame = _create_frame(
            HEADERS_FRAME,
            END_HEADER_AND_STREAM,
            stream_id=1,
            payload=payload,
        )

        await sock.sendall(begin_stream_frame)
        receive = await sock.recv(RECEIVE_SIZE)

        payload_size, frame_type, flags, stream_id, payload = _parse_frame_header(receive)
        assert payload_size == 4
        assert frame_type == RST_STREAM_FRAME
        assert flags == EMPTY_FLAGS
        assert stream_id == 1
        assert int.from_bytes(payload, byteorder='big') == PROTOCOL_ERROR_CODE

        valid_stream_frame = _create_frame(
            HEADERS_FRAME,
            END_HEADER_AND_STREAM,
            stream_id=3,
            payload=b''.join(_encode_header(k, v) for k, v in DEFAULT_HEADERS),
        )
        await sock.sendall(valid_stream_frame)
        receive = await sock.recv(RECEIVE_SIZE)

        _, frame_type, _, stream_id, _ = _parse_frame_header(receive)
        assert frame_type == HEADERS_FRAME
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
            await _receive_simple_response(sock, conn)

    assert differ.value_at('reset-streams') == 1


async def test_client_goaway_metric(create_connection, monitor_client, service_client):
    await service_client.update_server_state()

    async with monitor_client.metrics_diff(prefix='server.requests.http2') as differ:
        async with create_connection() as (sock, conn):
            conn.close_connection(error_code=0)
            await sock.sendall(conn.data_to_send())

    assert differ.value_at('goaway') == 1


async def test_stream_already_closed(create_connection, service_client):
    async with create_connection() as (sock, conn):

        async def open_and_close_simple_stream():
            stream_id = conn.get_next_available_stream_id()
            conn.send_headers(stream_id, DEFAULT_HEADERS)
            conn.end_stream(stream_id)
            await _receive_simple_response(sock, conn)
            return stream_id

        stream_id = await open_and_close_simple_stream()
        assert stream_id == 1

        reset_closed_stream = _create_frame(
            RST_STREAM_FRAME,
            EMPTY_FLAGS,
            stream_id,
            struct.pack('>I', PROTOCOL_ERROR_CODE),
        )

        # RST_STREAM for an already closed stream should not poison the connection.
        await sock.sendall(reset_closed_stream)
        await sock.sendall(reset_closed_stream)

        stream_id = await open_and_close_simple_stream()
        assert stream_id == 3


async def test_streams_with_the_same_id(create_connection, service_client):
    async with create_connection() as (sock, conn):
        stream_id = 1
        payload = b''.join(_encode_header(k, v) for k, v in DEFAULT_HEADERS)
        begin_stream_frame = _create_frame(
            HEADERS_FRAME,
            EMPTY_FLAGS,
            stream_id,
            payload,
        )
        await sock.sendall(begin_stream_frame)
        await sock.sendall(begin_stream_frame)
        receive = await sock.recv(RECEIVE_SIZE)

        assert GOAWAY_FRAME == receive[FRAME_TYPE_INDEX]  # GOAWAY frame
        assert 'unexpected non-CONTINUATION frame or stream_id is invalid' in str(
            receive,
        )
