import asyncio
import contextlib
import logging
import socket
import struct
import uuid

import h2.connection
import h2.events
import h2.settings
import pytest

DEFAULT_PATH = '/http2server'
DEFAULT_DATA = {'hello': 'world'}
DEFAULT_FRAME_SIZE = 1 << 14
RECEIVE_SIZE = 1 << 26
MAX_CONCURRENT_STREAMS = 100

DATA_FRAME = 0x0
HEADERS_FRAME = 0x01
GOAWAY_FRAME = 0x07
EMPTY_FLAGS = 0x0
END_HEADER_AND_STREAM = 0x05

FRAME_TYPE_INDEX = 3

DEFAULT_HEADERS = [
    (':method', 'GET'),
    (':path', f'{DEFAULT_PATH}?type=echo-header'),
    (':scheme', 'http'),
    (':authority', 'localhost'),
    ('echo-header', 'echo'),
]


async def test_http2_ping(http2_client):
    r = await http2_client.get('/ping', timeout=1)
    assert 200 == r.status_code
    assert '' == r.text


async def test_big_body(http2_client):
    s = 'x' * 2**22  # request - 4Mib. limit - 2Mib
    r = await http2_client.get(
        DEFAULT_PATH,
        params={'type': 'echo-body'},
        data=s,
    )
    assert 413 == r.status_code
    assert 'too large request' == r.text

    s = 'x' * 2**20  # request - 1Mib. limit - 2Mib
    r = await http2_client.get(
        DEFAULT_PATH,
        params={'type': 'echo-body'},
        data=s,
    )
    assert 200 == r.status_code
    assert s == r.text


async def test_body_different_size(http2_client):
    s = ''
    for _ in range(1026):
        r = await http2_client.get(
            DEFAULT_PATH,
            params={'type': 'echo-body'},
            data=s,
        )
        assert 200 == r.status_code
        assert s == r.text
        s += 'x'


async def test_json_body(http2_client):
    data = {'x': 'X', 'y': 'Y', 'd': 0.123, 'b': True, 'arr': [1, 2, 3, 4]}
    r = await http2_client.get(
        DEFAULT_PATH,
        params={'type': 'json'},
        json=data,
    )
    assert 200 == r.status_code
    assert data == r.json()


async def test_headers(http2_client):
    hval = 'val'
    r = await http2_client.post(
        DEFAULT_PATH,
        params={'type': 'echo-header'},
        headers={'echo-header': hval, 'test': 'test'},
        json=DEFAULT_DATA,
    )
    assert 200 == r.status_code
    assert hval == r.text


async def _get_metric(monitor_client, metric_name):
    metric = await monitor_client.single_metric(
        f'server.requests.http2.{metric_name}',
    )
    return metric.value


async def _request(client, req_per_client, count=1):
    for _ in range(req_per_client):
        data = str(uuid.uuid4())
        data *= count
        r = await client.put(
            DEFAULT_PATH,
            params={'type': 'echo-body'},
            data=data,
        )
        assert 200 == r.status_code
        assert data == r.text


async def test_concurrent_requests(
    http2_client,
    service_client,
    monitor_client,
):
    await service_client.update_server_state()

    current_streams = await _get_metric(monitor_client, 'streams-count')
    streams_parse_error = await _get_metric(
        monitor_client,
        'streams-parse-error',
    )
    clients_count = 10
    req_per_client = 100
    tasks = [_request(http2_client, req_per_client) for _ in range(clients_count)]
    await asyncio.gather(*tasks)

    await service_client.update_server_state()

    await asyncio.sleep(1.0)

    metrics = await monitor_client.metrics(prefix='server.requests.http2')
    assert len(metrics) == 5
    total_requests = clients_count * req_per_client + current_streams
    assert total_requests == await _get_metric(monitor_client, 'streams-count')
    assert total_requests == await _get_metric(monitor_client, 'streams-close')
    assert 0 == await _get_metric(monitor_client, 'reset-streams')
    assert 0 == await _get_metric(monitor_client, 'goaway')
    assert streams_parse_error == await _get_metric(
        monitor_client,
        'streams-parse-error',
    )


async def test_concurrent_requests_with_big_body(
    http2_client,
    service_client,
    monitor_client,
):
    await service_client.update_server_state()

    current_streams = await _get_metric(monitor_client, 'streams-count')
    streams_parse_error = await _get_metric(
        monitor_client,
        'streams-parse-error',
    )
    clients_count = 5
    req_per_client = 10
    count = int((2**20) / 128)  # 1Mib / size(uuid)
    tasks = [_request(http2_client, req_per_client, count) for _ in range(clients_count)]
    await asyncio.gather(*tasks)

    await service_client.update_server_state()

    metrics = await monitor_client.metrics(prefix='server.requests.http2')
    assert len(metrics) == 5
    total_requests = clients_count * req_per_client + current_streams
    assert total_requests == await _get_metric(monitor_client, 'streams-count')
    assert total_requests == await _get_metric(monitor_client, 'streams-close')
    assert 0 == await _get_metric(monitor_client, 'reset-streams')
    assert 0 == await _get_metric(monitor_client, 'goaway')
    assert streams_parse_error == await _get_metric(
        monitor_client,
        'streams-parse-error',
    )


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


async def test_http1_ping(service_client):
    r = await service_client.get('/ping')
    assert r.status == 200


async def test_http1_broken_bytes(service_client, create_socket):
    async with create_socket() as sock:
        await sock.sendall('GET / HTTP/1.1'.encode('utf-8'))
        with pytest.raises(asyncio.TimeoutError):
            await sock.recv(1024, timeout=1.0)
        await sock.sendall('garbage'.encode('utf-8'))
        r = await sock.recv(1024)
        assert 'HTTP/1.1 400 Bad Request' in r.decode('utf-8')


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
        while len(events) != 3:
            events += await _send_and_receive(sock, conn)
        e = events[0]
        assert isinstance(e, h2.events.RemoteSettingsChanged)
        assert MAX_CONCURRENT_STREAMS == e.changed_settings[3].new_value
        assert DEFAULT_FRAME_SIZE == e.changed_settings[5].new_value
        assert isinstance(events[1], h2.events.SettingsAcknowledged)
        assert max_streams == events[1].changed_settings[3].new_value
        assert isinstance(events[2], h2.events.SettingsAcknowledged)

        ping_data = '12345678'.encode()
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


def _assert_events(events):
    assert len(events) == 3
    assert isinstance(events[0], h2.events.ResponseReceived)
    assert isinstance(events[1], h2.events.DataReceived)
    assert isinstance(events[2], h2.events.StreamEnded)


async def _receive_simple_response(sock, conn):
    events = []
    while len(events) != 3:
        events += await _send_and_receive(sock, conn)
    _assert_events(events)


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

        _receive_simple_response(sock, conn)


def _encode_header(name, value):
    name_encoded = name.encode('utf-8')
    value_encoded = value.encode('utf-8')
    zero = struct.pack('>B', 0x0)
    return (
        zero + struct.pack('B', len(name_encoded)) + name_encoded + struct.pack('B', len(value_encoded)) + value_encoded
    )


def _assert_responses(events):
    assert len(events) % 3 == 0
    for i in range(0, len(events) - 3, 3):
        _assert_events(events[i : i + 3])


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
    expected_frames_count = MAX_CONCURRENT_STREAMS * 3  # 1 response =  (ResponseReceived, DataReceived, StreamEnded)
    while len(events) != expected_frames_count:
        receive = await sock.recv(RECEIVE_SIZE)
        if not receive:
            raise RuntimeError('Socket connection was closed by the other side')
        events += conn.receive_data(receive)
    _assert_responses(events)


@pytest.mark.skip(reason='TAXICOMMON-10258')
async def test_many_in_flight(
    create_connection,
    monitor_client,
    service_client,
):
    await service_client.update_server_state()

    assert await _get_metric(
        monitor_client,
        'streams-close',
    ) == await _get_metric(monitor_client, 'streams-count')

    async with create_connection() as (sock, conn):
        # The first spike
        await do_max_streams(sock, conn)
        # The second spike
        await do_max_streams(sock, conn)


@pytest.mark.skip(reason='TAXICOMMON-10258')
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
            EMPTY_FLAGS,
            stream_id,
            payload,
        )

        await sock.sendall(begin_stream_frame)
        receive = await sock.recv(RECEIVE_SIZE)

        assert GOAWAY_FRAME == receive[FRAME_TYPE_INDEX]  # GOAWAY frame
        assert 'request HEADERS: max concurrent streams exceeded' in str(receive)


@pytest.mark.skip(reason='TAXICOMMON-10232: broken in nghttp2 1.65.0')
async def test_stream_already_closed(create_connection, service_client):
    async with create_connection() as (sock, conn):

        async def open_and_close_simple_stream():
            stream_id = conn.get_next_available_stream_id()
            conn.send_headers(stream_id, DEFAULT_HEADERS)
            conn.end_stream(stream_id)
            _receive_simple_response(sock, conn)
            return stream_id

        stream_id = await open_and_close_simple_stream()
        assert stream_id == 1

        payload = b''.join(_encode_header(k, v) for k, v in DEFAULT_HEADERS)
        double_stream = _create_frame(
            HEADERS_FRAME,
            END_HEADER_AND_STREAM,
            stream_id + 1,
            payload,
        )
        _receive_simple_response(sock, conn)

        # Send the stream that is already closed => nghttp2 ignores these.
        await sock.sendall(double_stream)
        await sock.sendall(double_stream)

        stream_id = await open_and_close_simple_stream()
        # nghttp2 increments streams count on first double_stream => stream_id=3
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
