import struct
from typing import Any

import h2.connection
import h2.events

import testsuite.asyncio_socket

DEFAULT_FRAME_SIZE = 1 << 14
RECEIVE_SIZE = 1 << 26
MAX_CONCURRENT_STREAMS = 100
HTTP1_HEADERS_END = b'\r\n\r\n'

DATA_FRAME = 0x0
HEADERS_FRAME = 0x01
RST_STREAM_FRAME = 0x03
GOAWAY_FRAME = 0x07
PING_FRAME = 0x06
CONTINUATION_FRAME = 0x09
EMPTY_FLAGS = 0x0
ACK_FLAG = 0x01
END_STREAM = 0x01
END_HEADERS = 0x04
END_HEADER_AND_STREAM = 0x05
PROTOCOL_ERROR_CODE = 0x01

FRAME_TYPE_INDEX = 3

EVENTS_COUNT_IN_COMPLETED_STREAM = 3


def encode_header(name: str, value: str) -> bytes:
    name_encoded = name.encode('utf-8')
    value_encoded = value.encode('utf-8')
    zero = struct.pack('>B', 0x0)
    return (
        zero + struct.pack('B', len(name_encoded)) + name_encoded + struct.pack('B', len(value_encoded)) + value_encoded
    )


def create_frame(frame_type: int, flags: int, stream_id: int, payload: bytes) -> bytes:
    header = (
        struct.pack('>I', len(payload))[1:]
        + struct.pack('B', frame_type)
        + struct.pack('B', flags)
        + struct.pack('>I', stream_id & 0x7FFFFFFF)
    )
    assert len(header) == 9
    return header + payload


def parse_frame_header(frame: bytes) -> tuple[int, int, int, int, bytes]:
    payload_size = int.from_bytes(frame[:3], byteorder='big')
    stream_id = int.from_bytes(frame[5:9], byteorder='big') & 0x7FFFFFFF
    payload = frame[9 : 9 + payload_size]
    return payload_size, frame[3], frame[4], stream_id, payload


def is_ping_ack(frame: bytes, opaque_data: bytes) -> bool:
    if len(frame) < 9:
        return False
    _, frame_type, flags, stream_id, payload = parse_frame_header(frame)
    return frame_type == PING_FRAME and flags & ACK_FLAG and stream_id == 0 and payload == opaque_data


async def send_and_receive(
    sock: testsuite.asyncio_socket.AsyncioSocket,
    conn: h2.connection.H2Connection,
) -> list[Any]:
    await sock.sendall(conn.data_to_send())
    receive = await sock.recv(RECEIVE_SIZE)
    return conn.receive_data(receive)


def assert_is_completed_response(events: list[Any]) -> None:
    assert len(events) == EVENTS_COUNT_IN_COMPLETED_STREAM
    assert isinstance(events[0], h2.events.ResponseReceived)
    assert isinstance(events[1], h2.events.DataReceived)
    assert isinstance(events[2], h2.events.StreamEnded)


async def receive_simple_response(
    sock: testsuite.asyncio_socket.AsyncioSocket,
    conn: h2.connection.H2Connection,
) -> None:
    events = []
    while len(events) != EVENTS_COUNT_IN_COMPLETED_STREAM:
        events += await send_and_receive(sock, conn)
    assert_is_completed_response(events)


async def receive_until_stream_ended(
    sock: testsuite.asyncio_socket.AsyncioSocket,
    conn: h2.connection.H2Connection,
) -> list[Any]:
    events: list[Any] = []
    while not any(isinstance(event, h2.events.StreamEnded) for event in events):
        receive = await sock.recv(RECEIVE_SIZE)
        if not receive:
            raise RuntimeError('Socket connection was closed by the other side')
        events += conn.receive_data(receive)
    return events


def response_data(events: list[Any]) -> bytes:
    return b''.join(event.data for event in events if isinstance(event, h2.events.DataReceived))
