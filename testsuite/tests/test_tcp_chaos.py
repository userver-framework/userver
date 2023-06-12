import asyncio
import errno
import fcntl
import logging
import os
import socket
import time
import uuid

import pytest
from pytest_userver import chaos  # pylint: disable=import-error


_NOTICEABLE_DELAY = 0.5


logger = logging.getLogger(__name__)


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
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    fcntl.fcntl(sock, fcntl.F_SETFL, os.O_NONBLOCK)


async def _assert_data_from_to(
        sock_from: socket.socket, sock_to: socket.socket, loop,
) -> None:
    logger.debug('_assert_data_from_to sendall to %s', sock_from.getsockname())
    expected = b'pong_' + uuid.uuid4().bytes
    await loop.sock_sendall(sock_from, expected)
    logger.debug('_assert_data_from_to recv from %s', sock_to.getsockname())
    data = await loop.sock_recv(sock_to, len(expected))
    assert data == expected
    logger.debug('_assert_data_from_to done')


async def _assert_connection_dead(sock: socket.socket, loop) -> None:
    try:
        logger.debug('_assert_connection_dead starting')
        data = await loop.sock_recv(sock, 1)
        assert not data
    except ConnectionResetError:
        pass
    finally:
        logger.debug('_assert_connection_dead done')


async def _make_client(loop, gate: chaos.TcpGate):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    _make_nonblocking(sock)
    await loop.sock_connect(sock, gate.get_sockname_for_clients())
    return sock


class Server:
    def __init__(self, sock: socket.socket, loop):
        self._sock = sock
        self._loop = loop

    async def accept(self) -> socket.socket:
        server_connection, _ = await self._loop.sock_accept(self._sock)
        _make_nonblocking(server_connection)
        return server_connection

    def get_port(self) -> int:
        return self._sock.getsockname()[1]


@pytest.fixture(name='tcp_server', scope='function')
async def _server(loop):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    _make_nonblocking(sock)
    sock.bind(('localhost', 0))
    sock.listen()
    yield Server(sock, loop)
    sock.close()


@pytest.fixture(name='gate', scope='function')
async def _gate(loop, tcp_server):
    gate_config = chaos.GateRoute(
        name='tcp proxy',
        host_to_server='localhost',
        port_to_server=tcp_server.get_port(),
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture(name='tcp_client', scope='function')
async def _client(loop, gate):
    sock = await _make_client(loop, gate)
    yield sock
    sock.close()


@pytest.fixture(name='server_connection', scope='function')
async def _server_connection(loop, tcp_server, gate):
    sock = await tcp_server.accept()
    await gate.wait_for_connections(count=1)
    assert gate.connections_count() >= 1
    yield sock
    sock.close()


async def test_basic(tcp_client, gate, server_connection, loop):
    await _assert_data_from_to(tcp_client, server_connection, loop)
    await _assert_data_from_to(server_connection, tcp_client, loop)

    assert gate.connections_count() == 1
    await gate.sockets_close()
    assert gate.connections_count() == 0


async def test_to_client_noop(tcp_client, gate, server_connection, loop):
    gate.to_client_noop()

    await loop.sock_sendall(server_connection, b'ping')
    await _assert_data_from_to(tcp_client, server_connection, loop)
    assert not _has_data(tcp_client)

    gate.to_client_pass()
    hello = await loop.sock_recv(tcp_client, 4)
    assert hello == b'ping'
    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(tcp_client, server_connection, loop)


async def test_to_server_noop(tcp_client, gate, server_connection, loop):
    gate.to_server_noop()

    await loop.sock_sendall(tcp_client, b'ping')
    await _assert_data_from_to(server_connection, tcp_client, loop)
    assert not _has_data(server_connection)

    gate.to_server_pass()
    server_incoming_data = await loop.sock_recv(server_connection, 4)
    assert server_incoming_data == b'ping'
    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(tcp_client, server_connection, loop)


async def test_to_client_delay(tcp_client, gate, server_connection, loop):
    gate.to_client_delay(2 * _NOTICEABLE_DELAY)

    await _assert_data_from_to(tcp_client, server_connection, loop)

    start_time = time.monotonic()
    await _assert_data_from_to(server_connection, tcp_client, loop)
    assert time.monotonic() - start_time >= _NOTICEABLE_DELAY


async def test_to_server_delay(tcp_client, gate, server_connection, loop):
    gate.to_server_delay(2 * _NOTICEABLE_DELAY)

    await _assert_data_from_to(server_connection, tcp_client, loop)

    start_time = time.monotonic()
    await _assert_data_from_to(tcp_client, server_connection, loop)
    assert time.monotonic() - start_time >= _NOTICEABLE_DELAY


async def test_to_client_close_on_data(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    gate.to_client_close_on_data()
    tcp_client2 = await _make_client(loop, gate)
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(tcp_client, server_connection, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)
    assert gate.connections_count() == 2
    await loop.sock_sendall(server_connection, b'die')

    await _assert_connection_dead(server_connection, loop)
    await _assert_connection_dead(tcp_client, loop)

    gate.to_client_pass()
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)


async def test_to_server_close_on_data(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    gate.to_server_close_on_data()
    tcp_client2 = await _make_client(loop, gate)
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    assert gate.connections_count() == 2
    await loop.sock_sendall(tcp_client, b'die')

    await _assert_connection_dead(server_connection, loop)
    await _assert_connection_dead(tcp_client, loop)

    gate.to_server_pass()
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)


async def test_to_client_corrupt_data(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    gate.to_client_corrupt_data()
    tcp_client2 = await _make_client(loop, gate)
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(tcp_client, server_connection, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)
    assert gate.connections_count() == 2

    await loop.sock_sendall(server_connection, b'break me')
    data = await loop.sock_recv(tcp_client, 512)
    assert data
    assert data != b'break me'

    gate.to_client_pass()
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)
    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(tcp_client, server_connection, loop)


async def test_to_server_corrupt_data(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    gate.to_server_corrupt_data()
    tcp_client2 = await _make_client(loop, gate)
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    assert gate.connections_count() == 2

    await loop.sock_sendall(tcp_client, b'break me')
    data = await loop.sock_recv(server_connection, 512)
    assert data
    assert data != b'break me'

    gate.to_server_pass()
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)
    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(tcp_client, server_connection, loop)


async def test_to_client_limit_bps(tcp_client, gate, server_connection, loop):
    gate.to_client_limit_bps(2)

    start_time = time.monotonic()

    await loop.sock_sendall(server_connection, b'hello')
    data = await loop.sock_recv(tcp_client, 5)
    assert data
    assert data != b'hello'
    for _ in range(5):
        if data == b'hello':
            break
        data += await loop.sock_recv(tcp_client, 5)
    assert data == b'hello'

    assert time.monotonic() - start_time >= 1.0


async def test_to_server_limit_bps(tcp_client, gate, server_connection, loop):
    gate.to_server_limit_bps(2)

    start_time = time.monotonic()

    await loop.sock_sendall(tcp_client, b'hello')
    data = await loop.sock_recv(server_connection, 5)
    assert data
    assert data != b'hello'
    for _ in range(5):
        if data == b'hello':
            break
        data += await loop.sock_recv(server_connection, 5)
    assert data == b'hello'

    assert time.monotonic() - start_time >= 1.0


async def test_to_client_limit_time(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    gate.to_client_limit_time(_NOTICEABLE_DELAY, jitter=0.0)
    tcp_client2 = await _make_client(loop, gate)
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(tcp_client, server_connection, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)
    assert gate.connections_count() == 2

    await asyncio.sleep(_NOTICEABLE_DELAY)

    await _assert_data_from_to(server_connection, tcp_client, loop)
    await asyncio.sleep(_NOTICEABLE_DELAY * 2)
    await loop.sock_sendall(server_connection, b'die')
    await _assert_connection_dead(server_connection, loop)
    await _assert_connection_dead(tcp_client, loop)

    gate.to_client_pass()
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)


async def test_to_server_limit_time(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    gate.to_server_limit_time(_NOTICEABLE_DELAY, jitter=0.0)
    tcp_client2 = await _make_client(loop, gate)
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    assert gate.connections_count() == 2

    await asyncio.sleep(_NOTICEABLE_DELAY)

    await _assert_data_from_to(tcp_client, server_connection, loop)
    await asyncio.sleep(_NOTICEABLE_DELAY * 2)
    await loop.sock_sendall(tcp_client, b'die')
    await _assert_connection_dead(server_connection, loop)
    await _assert_connection_dead(tcp_client, loop)

    gate.to_server_pass()
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)


async def test_to_client_smaller_parts(
        tcp_client, gate, server_connection, loop,
):
    gate.to_client_smaller_parts(2)

    await loop.sock_sendall(server_connection, b'hello')
    data = await loop.sock_recv(tcp_client, 5)
    assert data
    assert len(data) < 5

    for _ in range(5):
        data += await loop.sock_recv(tcp_client, 5)
        if data == b'hello':
            break

    assert data == b'hello'


async def test_to_server_smaller_parts(
        tcp_client, gate, server_connection, loop,
):
    gate.to_server_smaller_parts(2)

    await loop.sock_sendall(tcp_client, b'hello')
    data = await loop.sock_recv(server_connection, 5)
    assert data
    assert len(data) < 5

    for _ in range(5):
        data += await loop.sock_recv(server_connection, 5)
        if data == b'hello':
            break

    assert data == b'hello'


async def test_to_client_concat(tcp_client, gate, server_connection, loop):
    gate.to_client_concat_packets(10)

    await loop.sock_sendall(server_connection, b'hello')
    await _assert_data_from_to(tcp_client, server_connection, loop)
    assert not _has_data(tcp_client)

    await loop.sock_sendall(server_connection, b'hello')
    data = await loop.sock_recv(tcp_client, 10)
    assert data == b'hellohello'

    gate.to_client_pass()
    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(tcp_client, server_connection, loop)


async def test_to_server_concat(tcp_client, gate, server_connection, loop):
    gate.to_server_concat_packets(10)

    await loop.sock_sendall(tcp_client, b'hello')
    await _assert_data_from_to(server_connection, tcp_client, loop)
    assert not _has_data(server_connection)

    await loop.sock_sendall(tcp_client, b'hello')
    data = await loop.sock_recv(server_connection, 10)
    assert data == b'hellohello'

    gate.to_server_pass()
    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(tcp_client, server_connection, loop)


async def test_to_client_limit_bytes(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    gate.to_client_limit_bytes(12)
    tcp_client2 = await _make_client(loop, gate)
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(tcp_client, server_connection, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)
    assert gate.connections_count() == 2

    await loop.sock_sendall(server_connection, b'hello')
    data = await loop.sock_recv(tcp_client, 10)
    assert data == b'hello'

    await loop.sock_sendall(server_connection2, b'die now')
    data = await loop.sock_recv(tcp_client2, 10)
    assert data == b'die now'

    await _assert_connection_dead(server_connection, loop)
    await _assert_connection_dead(tcp_client, loop)
    await _assert_connection_dead(server_connection2, loop)
    await _assert_connection_dead(tcp_client2, loop)

    # Check that limit is reset after closing socket
    tcp_client3 = await _make_client(loop, gate)
    server_connection3 = await tcp_server.accept()
    await _assert_data_from_to(tcp_client3, server_connection3, loop)
    assert gate.connections_count() == 1

    await loop.sock_sendall(server_connection3, b'XXXX die now')
    data = await loop.sock_recv(tcp_client3, 12)
    assert data == b'XXXX die now'

    await _assert_connection_dead(server_connection3, loop)
    await _assert_connection_dead(tcp_client3, loop)


async def test_to_server_limit_bytes(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    gate.to_server_limit_bytes(8)
    tcp_client2 = await _make_client(loop, gate)
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    assert gate.connections_count() == 2

    await loop.sock_sendall(tcp_client, b'hello')
    data = await loop.sock_recv(server_connection, 10)
    assert data == b'hello'

    await loop.sock_sendall(tcp_client2, b'die')
    data = await loop.sock_recv(server_connection2, 10)
    assert data == b'die'

    await _assert_connection_dead(server_connection, loop)
    await _assert_connection_dead(tcp_client, loop)
    await _assert_connection_dead(server_connection2, loop)
    await _assert_connection_dead(tcp_client2, loop)


async def test_substitute(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    gate.to_server_substitute('hello', 'die')
    gate.to_client_substitute('hello', 'die')

    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(tcp_client, server_connection, loop)

    await loop.sock_sendall(tcp_client, b'hello')
    data = await loop.sock_recv(server_connection, 10)
    assert data == b'die'

    await loop.sock_sendall(server_connection, b'hello')
    data = await loop.sock_recv(tcp_client, 10)
    assert data == b'die'


async def test_wait_for_connections(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    assert gate.connections_count() == 1
    await gate.wait_for_connections(count=1, timeout=_NOTICEABLE_DELAY)

    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(tcp_client, server_connection, loop)

    try:
        await gate.wait_for_connections(count=2, timeout=_NOTICEABLE_DELAY)
        assert False
    except asyncio.TimeoutError:
        pass

    tcp_client2 = await _make_client(loop, gate)
    server_connection2 = await tcp_server.accept()

    await gate.wait_for_connections(count=2, timeout=_NOTICEABLE_DELAY)
    assert gate.connections_count() == 2

    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(tcp_client, server_connection, loop)
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)


async def test_start_stop_accepting(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    await gate.stop_accepting()

    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(tcp_client, server_connection, loop)

    tcp_client2 = await _make_client(loop, gate)

    try:
        await asyncio.wait_for(
            asyncio.create_task(tcp_server.accept()),
            timeout=_NOTICEABLE_DELAY,
        )
        assert False
    except asyncio.TimeoutError:
        pass

    gate.start_accepting()

    server_connection2 = await tcp_server.accept()
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)
    await _assert_data_from_to(server_connection, tcp_client, loop)
    await _assert_data_from_to(tcp_client, server_connection, loop)


async def test_start_stop_gate(
        tcp_client, gate, server_connection, tcp_server, loop,
):
    assert gate.connections_count() == 1
    await gate.stop()
    assert gate.connections_count() == 0

    try:
        await _make_client(loop, gate)
        assert False
    except ConnectionRefusedError:
        pass

    try:
        await asyncio.wait_for(
            asyncio.create_task(tcp_server.accept()),
            timeout=_NOTICEABLE_DELAY,
        )
        assert False
    except asyncio.TimeoutError:
        pass

    gate.start()
    tcp_client2 = await _make_client(loop, gate)
    server_connection2 = await tcp_server.accept()
    await _assert_data_from_to(server_connection2, tcp_client2, loop)
    await _assert_data_from_to(tcp_client2, server_connection2, loop)
