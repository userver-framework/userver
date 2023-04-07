import asyncio
import errno
import fcntl
import os
import socket
import time
import typing
import uuid

import pytest
from pytest_userver import chaos  # pylint: disable=import-error


_NOTICEABLE_DELAY = 0.5
TEST_TIMEOUT = 5
RECV_MAX_SIZE = 4096


def _has_data(recv_socket: socket.socket) -> bool:
    try:
        data = recv_socket.recv(1, socket.MSG_PEEK)
        return bool(data)
    except socket.error as socket_error:
        err = socket_error.args[0]
        if err in {errno.EAGAIN, errno.EWOULDBLOCK}:
            return False
        raise socket_error


def _make_nonblocking(sock: socket.socket) -> None:
    fcntl.fcntl(sock, fcntl.F_SETFL, os.O_NONBLOCK)


async def _assert_data_from_to(
        sock_from: socket.socket, sock_to: socket.socket, loop,
) -> None:
    expected = b'pong_' + uuid.uuid4().bytes
    await loop.sock_sendall(sock_from, expected)
    data = await loop.sock_recv(sock_to, len(expected))
    assert data == expected


async def _assert_receive_timeout(sock: socket.socket, loop) -> None:
    try:
        data = await asyncio.wait_for(
            loop.sock_recv(sock, 1), _NOTICEABLE_DELAY,
        )
        assert not data
    except asyncio.TimeoutError:
        pass


async def _make_client(loop, gate: chaos.UdpGate):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    _make_nonblocking(sock)
    await loop.sock_connect(sock, gate.get_sockname_for_clients())
    return sock


class UdpServer:
    def __init__(self, sock: socket.socket, loop):
        self._sock = sock
        self._loop = loop

    async def accept(self, client: socket.socket) -> socket.socket:

        # Have to recreate socket, as socket cannot be disconnected on macOS
        server_port = self._sock.getsockname()[1]
        self._sock.close()
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        _make_nonblocking(self._sock)

        self._sock.bind(('localhost', server_port))

        await self._loop.sock_sendall(client, b'ping')
        await chaos._wait_for_message_task(  # pylint: disable=protected-access
            self._sock,
        )
        msg, addr = self._sock.recvfrom(RECV_MAX_SIZE)

        self._sock.connect(addr)
        assert msg == b'ping'
        return self._sock

    def get_port(self) -> int:
        return self._sock.getsockname()[1]


@pytest.fixture(name='udp_server', scope='function')
async def _server(loop):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    _make_nonblocking(sock)
    sock.bind(('localhost', 0))
    try:
        yield UdpServer(sock, loop)
    finally:
        sock.close()


@pytest.fixture(name='gate', scope='function')
async def _gate(loop, udp_server):
    gate_config = chaos.GateRoute(
        name='udp proxy',
        host_to_server='127.0.0.1',
        port_to_server=udp_server.get_port(),
    )
    async with chaos.UdpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture(name='udp_client', scope='function')
async def _client(loop, gate):
    sock = await _make_client(loop, gate)
    try:
        yield sock
    finally:
        sock.close()


@pytest.fixture(name='server_socket', scope='function')
async def _server_socket(loop, udp_client, udp_server, gate):
    sock = await udp_server.accept(udp_client)
    assert gate.is_connected()
    try:
        yield sock
    finally:
        sock.close()


async def test_basic(udp_client, gate, server_socket, loop):
    await _assert_data_from_to(udp_client, server_socket, loop)
    await _assert_data_from_to(server_socket, udp_client, loop)

    assert gate.is_connected()
    await gate.sockets_close()
    assert not gate.is_connected()


async def test_to_client_udp_message_queue(
        udp_client, gate, server_socket, loop,
):
    messages_count = 5
    for i in range(messages_count):
        await loop.sock_sendall(
            server_socket, b'message' + i.to_bytes(1, 'big'),
        )

    for i in range(messages_count):
        assert await loop.sock_recv(
            udp_client, RECV_MAX_SIZE,
        ) == b'message' + i.to_bytes(1, 'big')


async def test_to_server_udp_message_queue(
        udp_client, gate, server_socket, loop,
):
    messages_count = 5
    for i in range(messages_count):
        await loop.sock_sendall(udp_client, b'message' + i.to_bytes(1, 'big'))

    for i in range(messages_count):
        assert await loop.sock_recv(
            server_socket, RECV_MAX_SIZE,
        ) == b'message' + i.to_bytes(1, 'big')


async def test_fifo_udp_message_queue(udp_client, gate, server_socket, loop):
    messages_count = 5
    for i in range(messages_count):
        await loop.sock_sendall(udp_client, b'message' + i.to_bytes(1, 'big'))

    for i in range(messages_count):
        await loop.sock_sendall(
            server_socket, b'message' + i.to_bytes(1, 'big'),
        )

    for i in range(messages_count):
        assert await loop.sock_recv(
            udp_client, RECV_MAX_SIZE,
        ) == b'message' + i.to_bytes(1, 'big')

    for i in range(messages_count):
        assert await loop.sock_recv(
            server_socket, RECV_MAX_SIZE,
        ) == b'message' + i.to_bytes(1, 'big')


async def test_to_server_parallel_udp_message(
        udp_client, gate, server_socket, loop,
):
    messages_count = 100

    tasks: typing.List[typing.Awaitable] = []
    for i in range(messages_count):
        tasks.append(
            loop.sock_sendall(udp_client, b'message' + i.to_bytes(1, 'big')),
        )

    await asyncio.gather(*tasks)

    res: typing.List[bool] = [False] * messages_count
    for _ in range(messages_count):
        message = await loop.sock_recv(server_socket, RECV_MAX_SIZE)
        assert message[:7] == b'message'
        idx = int.from_bytes(message[7:], 'big')
        res[idx] = True

    assert all(res)


async def test_to_client_noop(udp_client, gate, server_socket, loop):
    gate.to_client_noop()

    await loop.sock_sendall(server_socket, b'ping')
    await _assert_data_from_to(udp_client, server_socket, loop)
    assert not _has_data(udp_client)

    gate.to_client_pass()
    hello = await loop.sock_recv(udp_client, 4)
    assert hello == b'ping'
    await _assert_data_from_to(server_socket, udp_client, loop)
    await _assert_data_from_to(udp_client, server_socket, loop)


async def test_to_server_noop(udp_client, gate, server_socket, loop):
    gate.to_server_noop()

    await loop.sock_sendall(udp_client, b'ping')
    await _assert_data_from_to(server_socket, udp_client, loop)
    assert not _has_data(server_socket)

    gate.to_server_pass()
    server_incoming_data = await loop.sock_recv(server_socket, 4)
    assert server_incoming_data == b'ping'
    await _assert_data_from_to(server_socket, udp_client, loop)
    await _assert_data_from_to(udp_client, server_socket, loop)


async def test_to_client_delay(udp_client, gate, server_socket, loop):
    gate.to_client_delay(2 * _NOTICEABLE_DELAY)

    await _assert_data_from_to(udp_client, server_socket, loop)

    start_time = time.monotonic()
    await _assert_data_from_to(server_socket, udp_client, loop)
    assert time.monotonic() - start_time >= _NOTICEABLE_DELAY


async def test_to_server_delay(udp_client, gate, server_socket, loop):
    gate.to_server_delay(2 * _NOTICEABLE_DELAY)

    await _assert_data_from_to(server_socket, udp_client, loop)

    start_time = time.monotonic()
    await _assert_data_from_to(udp_client, server_socket, loop)
    assert time.monotonic() - start_time >= _NOTICEABLE_DELAY


async def test_to_client_close_on_data(
        udp_client, gate, server_socket, udp_server, loop,
):
    gate.to_client_close_on_data()

    await _assert_data_from_to(udp_client, server_socket, loop)
    assert gate.is_connected()
    await loop.sock_sendall(server_socket, b'die')

    await _assert_receive_timeout(udp_client, loop)


async def test_to_server_close_on_data(
        udp_client, gate, server_socket, udp_server, loop,
):
    gate.to_server_close_on_data()

    await _assert_data_from_to(server_socket, udp_client, loop)
    assert gate.is_connected()
    await loop.sock_sendall(udp_client, b'die')

    await _assert_receive_timeout(server_socket, loop)


async def test_to_client_corrupt_data(
        udp_client, gate, server_socket, udp_server, loop,
):
    gate.to_client_corrupt_data()

    await _assert_data_from_to(udp_client, server_socket, loop)
    assert gate.is_connected()

    await loop.sock_sendall(server_socket, b'break me')
    data = await loop.sock_recv(udp_client, 512)
    assert data
    assert data != b'break me'

    gate.to_client_pass()
    await _assert_data_from_to(server_socket, udp_client, loop)
    await _assert_data_from_to(udp_client, server_socket, loop)


async def test_to_server_corrupt_data(
        udp_client, gate, server_socket, udp_server, loop,
):
    gate.to_server_corrupt_data()

    await _assert_data_from_to(server_socket, udp_client, loop)
    assert gate.is_connected()

    await loop.sock_sendall(udp_client, b'break me')
    data = await loop.sock_recv(server_socket, 512)
    assert data
    assert data != b'break me'

    gate.to_server_pass()
    await _assert_data_from_to(server_socket, udp_client, loop)
    await _assert_data_from_to(udp_client, server_socket, loop)


async def test_to_client_limit_bps(udp_client, gate, server_socket, loop):
    gate.to_client_limit_bps(2)

    start_time = time.monotonic()

    message = b'hello'

    # udp sockets drops message ending, so we have only the part what
    # was read on first try
    for _ in range(5):
        await loop.sock_sendall(server_socket, message)
        data = await loop.sock_recv(udp_client, 5)
        assert data
        assert data != message and message.startswith(data)

    assert time.monotonic() - start_time >= 1.0


async def test_to_server_limit_bps(udp_client, gate, server_socket, loop):
    gate.to_server_limit_bps(2)

    start_time = time.monotonic()

    message = b'hello'

    # udp sockets drops message ending, so we have only the
    # part what was read on first try
    for _ in range(5):
        await loop.sock_sendall(udp_client, b'hello')
        data = await loop.sock_recv(server_socket, 5)
        assert data
        assert len(data) and message.startswith(data)

    assert time.monotonic() - start_time >= 1.0


async def test_to_client_limit_time(
        udp_client, gate, server_socket, udp_server, loop,
):
    gate.to_client_limit_time(_NOTICEABLE_DELAY, jitter=0.0)

    await _assert_data_from_to(udp_client, server_socket, loop)
    assert gate.is_connected()

    await asyncio.sleep(_NOTICEABLE_DELAY)

    await _assert_data_from_to(server_socket, udp_client, loop)
    await asyncio.sleep(_NOTICEABLE_DELAY * 2)
    await loop.sock_sendall(server_socket, b'die')
    await _assert_receive_timeout(udp_client, loop)


async def test_to_server_limit_time(
        udp_client, gate, server_socket, udp_server, loop,
):
    gate.to_server_limit_time(_NOTICEABLE_DELAY, jitter=0.0)

    await _assert_data_from_to(server_socket, udp_client, loop)
    assert gate.is_connected()

    await asyncio.sleep(_NOTICEABLE_DELAY)

    await _assert_data_from_to(udp_client, server_socket, loop)
    await asyncio.sleep(_NOTICEABLE_DELAY * 2)
    await loop.sock_sendall(udp_client, b'die')
    await _assert_receive_timeout(server_socket, loop)


async def test_to_client_limit_bytes(
        udp_client, gate, server_socket, udp_server, loop,
):
    gate.to_client_limit_bytes(14)

    await _assert_data_from_to(udp_client, server_socket, loop)
    assert gate.is_connected()

    await loop.sock_sendall(server_socket, b'hello')
    data = await loop.sock_recv(udp_client, 10)
    assert data == b'hello'

    await loop.sock_sendall(server_socket, b'die after')
    data = await loop.sock_recv(udp_client, 10)
    assert data == b'die after'

    await loop.sock_sendall(server_socket, b'dead now')

    await _assert_receive_timeout(udp_client, loop)


async def test_to_server_limit_bytes(
        udp_client, gate, server_socket, udp_server, loop,
):
    gate.to_server_limit_bytes(14)

    await _assert_data_from_to(server_socket, udp_client, loop)
    assert gate.is_connected()

    await loop.sock_sendall(udp_client, b'hello')
    data = await loop.sock_recv(server_socket, 10)
    assert data == b'hello'

    await loop.sock_sendall(udp_client, b'die after')
    data = await loop.sock_recv(server_socket, 10)
    assert data == b'die after'

    await _assert_receive_timeout(server_socket, loop)


async def test_substitute(udp_client, gate, server_socket, udp_server, loop):
    gate.to_server_substitute('hello', 'die')
    gate.to_client_substitute('hello', 'die')

    await _assert_data_from_to(server_socket, udp_client, loop)
    await _assert_data_from_to(udp_client, server_socket, loop)

    await loop.sock_sendall(udp_client, b'hello')
    data = await loop.sock_recv(server_socket, 10)
    assert data == b'die'

    await loop.sock_sendall(server_socket, b'hello')
    data = await loop.sock_recv(udp_client, 10)
    assert data == b'die'


async def test_wait_for_connections(
        udp_client, gate, server_socket, udp_server, loop,
):
    assert gate.is_connected()
    with pytest.raises(AttributeError):
        await gate.wait_for_connections(count=1, timeout=_NOTICEABLE_DELAY)


async def test_start_stop(udp_client, gate, server_socket, udp_server, loop):
    await gate.stop()

    await loop.sock_sendall(udp_client, b'hello')
    await _assert_receive_timeout(server_socket, loop)

    gate.start()
    udp_client2 = await _make_client(loop, gate)
    server_socket2 = await udp_server.accept(udp_client2)

    await _assert_data_from_to(server_socket2, udp_client2, loop)
    await _assert_data_from_to(udp_client2, server_socket2, loop)
