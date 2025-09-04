import asyncio
import logging
import socket
import time
import uuid

import pytest
from pytest_userver import chaos  # pylint: disable=import-error

from testsuite.asyncio_socket import AsyncioSocket
from testsuite.asyncio_socket import AsyncioSocketsFactory

_NOTICEABLE_DELAY = 0.5


logger = logging.getLogger(__name__)


async def _assert_data_from_to(
    sock_from: AsyncioSocket,
    sock_to: AsyncioSocket,
) -> None:
    logger.debug('_assert_data_from_to sendall to %s', sock_from.getsockname())
    expected = b'pong_' + uuid.uuid4().bytes
    await sock_from.sendall(expected)
    logger.debug('_assert_data_from_to recv from %s', sock_to.getsockname())
    data = await sock_to.recv(len(expected))
    assert data == expected
    logger.debug('_assert_data_from_to done')


async def _assert_receive_timeout(sock: AsyncioSocket) -> None:
    try:
        data = await sock.recv(1, timeout=_NOTICEABLE_DELAY)
        assert not data
    except asyncio.TimeoutError:
        pass


async def _assert_connection_dead(sock: AsyncioSocket) -> None:
    try:
        logger.debug('_assert_connection_dead starting')
        data = await sock.recv(1)
        assert not data
    except ConnectionResetError:
        pass
    finally:
        logger.debug('_assert_connection_dead done')


class Server:
    def __init__(self, sock: AsyncioSocket):
        self._sock = sock

    async def accept(self) -> AsyncioSocket:
        server_connection, _ = await self._sock.accept()
        return server_connection

    def get_port(self) -> int:
        return self._sock.getsockname()[1]


@pytest.fixture(name='make_client')
async def _make_client(
    gate: chaos.TcpGate,
    asyncio_socket: AsyncioSocketsFactory,
):
    async def make_client():
        sock = asyncio_socket.tcp()
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        await sock.connect(gate.get_sockname_for_clients())
        return sock

    return make_client


@pytest.fixture(name='tcp_server')
async def _server(asyncio_socket: AsyncioSocketsFactory):
    sock = asyncio_socket.tcp()
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('localhost', 0))
    sock.listen()
    yield Server(sock)
    sock.close()


@pytest.fixture(name='gate', scope='function')
async def _gate(tcp_server):
    gate_config = chaos.GateRoute(
        name='tcp proxy',
        host_to_server='localhost',
        port_to_server=tcp_server.get_port(),
    )
    async with chaos.TcpGate(gate_config) as proxy:
        yield proxy


@pytest.fixture(name='tcp_client', scope='function')
async def _client(make_client):
    sock = await make_client()
    yield sock
    sock.close()


@pytest.fixture(name='server_connection', scope='function')
async def _server_connection(tcp_server, gate):
    sock = await tcp_server.accept()
    await gate.wait_for_connections(count=1)
    assert gate.connections_count() >= 1
    yield sock
    sock.close()


async def test_basic(tcp_client, gate, server_connection):
    await _assert_data_from_to(tcp_client, server_connection)
    await _assert_data_from_to(server_connection, tcp_client)

    assert gate.connections_count() == 1
    await gate.sockets_close()
    assert gate.connections_count() == 0


async def test_to_client_noop(tcp_client, gate, server_connection):
    await gate.to_client_noop()

    await server_connection.sendall(b'ping')
    await _assert_data_from_to(tcp_client, server_connection)
    assert not tcp_client.has_data()

    await gate.to_client_pass()
    hello = await tcp_client.recv(4)
    assert hello == b'ping'
    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(tcp_client, server_connection)


async def test_to_server_noop(tcp_client, gate, server_connection):
    await gate.to_server_noop()

    await tcp_client.sendall(b'ping')
    await _assert_data_from_to(server_connection, tcp_client)
    assert not server_connection.has_data()

    await gate.to_server_pass()
    server_incoming_data = await server_connection.recv(4)
    assert server_incoming_data == b'ping'
    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(tcp_client, server_connection)


async def test_to_client_drop(tcp_client, gate, server_connection):
    await gate.to_client_drop()

    await server_connection.sendall(b'ping')
    await _assert_data_from_to(tcp_client, server_connection)
    assert not tcp_client.has_data()

    await gate.to_client_pass()
    await _assert_receive_timeout(tcp_client)


async def test_to_server_drop(tcp_client, gate, server_connection):
    await gate.to_server_drop()

    await tcp_client.sendall(b'ping')
    await _assert_data_from_to(server_connection, tcp_client)
    assert not server_connection.has_data()

    await gate.to_server_pass()
    await _assert_receive_timeout(server_connection)


async def test_to_client_delay(tcp_client, gate, server_connection):
    await gate.to_client_delay(2 * _NOTICEABLE_DELAY)

    await _assert_data_from_to(tcp_client, server_connection)

    start_time = time.monotonic()
    await _assert_data_from_to(server_connection, tcp_client)
    assert time.monotonic() - start_time >= _NOTICEABLE_DELAY


async def test_to_server_delay(tcp_client, gate, server_connection):
    await gate.to_server_delay(2 * _NOTICEABLE_DELAY)

    await _assert_data_from_to(server_connection, tcp_client)

    start_time = time.monotonic()
    await _assert_data_from_to(tcp_client, server_connection)
    assert time.monotonic() - start_time >= _NOTICEABLE_DELAY


async def test_to_client_close_on_data(
    tcp_client,
    gate,
    make_client,
    server_connection,
    tcp_server,
):
    await gate.to_client_close_on_data()
    tcp_client2 = await make_client()
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(tcp_client, server_connection)
    await _assert_data_from_to(tcp_client2, server_connection2)
    assert gate.connections_count() == 2
    await server_connection.sendall(b'die')

    await _assert_connection_dead(server_connection)
    await _assert_connection_dead(tcp_client)

    await gate.to_client_pass()
    await _assert_data_from_to(server_connection2, tcp_client2)
    await _assert_data_from_to(tcp_client2, server_connection2)


async def test_to_server_close_on_data(
    tcp_client,
    gate,
    make_client,
    server_connection,
    tcp_server,
):
    await gate.to_server_close_on_data()
    tcp_client2 = await make_client()
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(server_connection2, tcp_client2)
    assert gate.connections_count() == 2
    await tcp_client.sendall(b'die')

    await _assert_connection_dead(server_connection)
    await _assert_connection_dead(tcp_client)

    await gate.to_server_pass()
    await _assert_data_from_to(server_connection2, tcp_client2)
    await _assert_data_from_to(tcp_client2, server_connection2)


async def test_to_client_corrupt_data(
    tcp_client,
    gate,
    make_client,
    server_connection,
    tcp_server,
):
    await gate.to_client_corrupt_data()
    tcp_client2 = await make_client()
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(tcp_client, server_connection)
    await _assert_data_from_to(tcp_client2, server_connection2)
    assert gate.connections_count() == 2

    await server_connection.sendall(b'break me')
    data = await tcp_client.recv(512)
    assert data
    assert data != b'break me'

    await gate.to_client_pass()
    await _assert_data_from_to(server_connection2, tcp_client2)
    await _assert_data_from_to(tcp_client2, server_connection2)
    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(tcp_client, server_connection)


async def test_to_server_corrupt_data(
    tcp_client,
    gate,
    make_client,
    server_connection,
    tcp_server,
):
    await gate.to_server_corrupt_data()
    tcp_client2 = await make_client()
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(server_connection2, tcp_client2)
    assert gate.connections_count() == 2

    await tcp_client.sendall(b'break me')
    data = await server_connection.recv(512)
    assert data
    assert data != b'break me'

    await gate.to_server_pass()
    await _assert_data_from_to(server_connection2, tcp_client2)
    await _assert_data_from_to(tcp_client2, server_connection2)
    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(tcp_client, server_connection)


async def test_to_client_limit_bps(
    tcp_client,
    gate,
    make_client,
    server_connection,
):
    await gate.to_client_limit_bps(2)

    start_time = time.monotonic()

    await server_connection.sendall(b'hello')
    data = await tcp_client.recv(5)
    assert data
    assert data != b'hello'
    for _ in range(5):
        if data == b'hello':
            break
        data += await tcp_client.recv(5)
    assert data == b'hello'

    assert time.monotonic() - start_time >= 1.0


async def test_to_server_limit_bps(
    tcp_client,
    gate,
    make_client,
    server_connection,
):
    await gate.to_server_limit_bps(2)

    start_time = time.monotonic()

    await tcp_client.sendall(b'hello')
    data = await server_connection.recv(5)
    assert data
    assert data != b'hello'
    for _ in range(5):
        if data == b'hello':
            break
        data += await server_connection.recv(5)
    assert data == b'hello'

    assert time.monotonic() - start_time >= 1.0


async def test_to_client_limit_time(
    tcp_client,
    gate,
    make_client,
    server_connection,
    tcp_server,
):
    await gate.to_client_limit_time(_NOTICEABLE_DELAY, jitter=0.0)
    tcp_client2 = await make_client()
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(tcp_client, server_connection)
    await _assert_data_from_to(tcp_client2, server_connection2)
    assert gate.connections_count() == 2

    await asyncio.sleep(_NOTICEABLE_DELAY)

    await _assert_data_from_to(server_connection, tcp_client)
    await asyncio.sleep(_NOTICEABLE_DELAY * 2)
    await server_connection.sendall(b'die')
    await _assert_connection_dead(server_connection)
    await _assert_connection_dead(tcp_client)

    await gate.to_client_pass()
    await _assert_data_from_to(server_connection2, tcp_client2)
    await _assert_data_from_to(tcp_client2, server_connection2)


async def test_to_server_limit_time(
    tcp_client,
    gate,
    make_client,
    server_connection,
    tcp_server,
):
    await gate.to_server_limit_time(_NOTICEABLE_DELAY, jitter=0.0)
    tcp_client2 = await make_client()
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(server_connection2, tcp_client2)
    assert gate.connections_count() == 2

    await asyncio.sleep(_NOTICEABLE_DELAY)

    await _assert_data_from_to(tcp_client, server_connection)
    await asyncio.sleep(_NOTICEABLE_DELAY * 2)
    await tcp_client.sendall(b'die')
    await _assert_connection_dead(server_connection)
    await _assert_connection_dead(tcp_client)

    await gate.to_server_pass()
    await _assert_data_from_to(server_connection2, tcp_client2)
    await _assert_data_from_to(tcp_client2, server_connection2)


async def test_to_client_smaller_parts(
    tcp_client,
    gate,
    make_client,
    server_connection,
):
    await gate.to_client_smaller_parts(2)

    await server_connection.sendall(b'hello')
    data = await tcp_client.recv(5)
    assert data
    assert len(data) < 5

    for _ in range(5):
        data += await tcp_client.recv(5)
        if data == b'hello':
            break

    assert data == b'hello'


async def test_to_server_smaller_parts(
    tcp_client,
    gate,
    make_client,
    server_connection,
):
    await gate.to_server_smaller_parts(2)

    await tcp_client.sendall(b'hello')
    data = await server_connection.recv(5)
    assert data
    assert len(data) < 5

    for _ in range(5):
        data += await server_connection.recv(5)
        if data == b'hello':
            break

    assert data == b'hello'


async def test_to_client_concat(tcp_client, gate, server_connection):
    await gate.to_client_concat_packets(10)

    await server_connection.sendall(b'hello')
    await _assert_data_from_to(tcp_client, server_connection)
    assert not tcp_client.has_data()

    await server_connection.sendall(b'hello')
    data = await tcp_client.recv(10)
    assert data == b'hellohello'

    await gate.to_client_pass()
    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(tcp_client, server_connection)


async def test_to_server_concat(tcp_client, gate, server_connection):
    await gate.to_server_concat_packets(10)

    await tcp_client.sendall(b'hello')
    await _assert_data_from_to(server_connection, tcp_client)
    assert not server_connection.has_data()

    await tcp_client.sendall(b'hello')
    data = await server_connection.recv(10)
    assert data == b'hellohello'

    await gate.to_server_pass()
    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(tcp_client, server_connection)


async def test_to_client_limit_bytes(
    tcp_client,
    gate,
    make_client,
    server_connection,
    tcp_server,
):
    await gate.to_client_limit_bytes(12)
    tcp_client2 = await make_client()
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(tcp_client, server_connection)
    await _assert_data_from_to(tcp_client2, server_connection2)
    assert gate.connections_count() == 2

    await server_connection.sendall(b'hello')
    data = await tcp_client.recv(10)
    assert data == b'hello'

    await server_connection2.sendall(b'die now')
    data = await tcp_client2.recv(10)
    assert data == b'die now'

    await _assert_connection_dead(server_connection)
    await _assert_connection_dead(tcp_client)
    await _assert_connection_dead(server_connection2)
    await _assert_connection_dead(tcp_client2)

    # Check that limit is reset after closing socket
    tcp_client3 = await make_client()
    server_connection3 = await tcp_server.accept()
    await _assert_data_from_to(tcp_client3, server_connection3)
    assert gate.connections_count() == 1

    await server_connection3.sendall(b'XXXX die now')
    data = await tcp_client3.recv(12)
    assert data == b'XXXX die now'

    await _assert_connection_dead(server_connection3)
    await _assert_connection_dead(tcp_client3)


async def test_to_server_limit_bytes(
    tcp_client,
    gate,
    make_client,
    server_connection,
    tcp_server,
):
    await gate.to_server_limit_bytes(8)
    tcp_client2 = await make_client()
    server_connection2 = await tcp_server.accept()

    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(server_connection2, tcp_client2)
    assert gate.connections_count() == 2

    await tcp_client.sendall(b'hello')
    data = await server_connection.recv(10)
    assert data == b'hello'

    await tcp_client2.sendall(b'die')
    data = await server_connection2.recv(10)
    assert data == b'die'

    await _assert_connection_dead(server_connection)
    await _assert_connection_dead(tcp_client)
    await _assert_connection_dead(server_connection2)
    await _assert_connection_dead(tcp_client2)


async def test_substitute(
    tcp_client,
    gate,
    make_client,
    server_connection,
    tcp_server,
):
    await gate.to_server_substitute('hello', 'die')
    await gate.to_client_substitute('hello', 'die')

    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(tcp_client, server_connection)

    await tcp_client.sendall(b'hello')
    data = await server_connection.recv(10)
    assert data == b'die'

    await server_connection.sendall(b'hello')
    data = await tcp_client.recv(10)
    assert data == b'die'


async def test_wait_for_connections(
    tcp_client,
    gate,
    make_client,
    server_connection,
    tcp_server,
):
    assert gate.connections_count() == 1
    await gate.wait_for_connections(count=1, timeout=_NOTICEABLE_DELAY)

    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(tcp_client, server_connection)

    with pytest.raises(asyncio.TimeoutError):
        await gate.wait_for_connections(count=2, timeout=_NOTICEABLE_DELAY)

    tcp_client2 = await make_client()
    server_connection2 = await tcp_server.accept()

    await gate.wait_for_connections(count=2, timeout=_NOTICEABLE_DELAY)
    assert gate.connections_count() == 2

    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(tcp_client, server_connection)
    await _assert_data_from_to(server_connection2, tcp_client2)
    await _assert_data_from_to(tcp_client2, server_connection2)


async def test_start_stop_accepting(
    tcp_client,
    make_client,
    gate,
    server_connection,
    tcp_server,
):
    await gate.stop_accepting()

    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(tcp_client, server_connection)

    tcp_client2 = await make_client()

    with pytest.raises(asyncio.TimeoutError):
        await asyncio.wait_for(
            asyncio.create_task(tcp_server.accept()),
            timeout=_NOTICEABLE_DELAY,
        )

    gate.start_accepting()

    server_connection2 = await tcp_server.accept()
    await _assert_data_from_to(server_connection2, tcp_client2)
    await _assert_data_from_to(tcp_client2, server_connection2)
    await _assert_data_from_to(server_connection, tcp_client)
    await _assert_data_from_to(tcp_client, server_connection)


async def test_start_stop_gate(
    tcp_client,
    make_client,
    gate,
    server_connection,
    tcp_server,
):
    assert gate.connections_count() == 1
    await gate.stop()
    assert gate.connections_count() == 0

    with pytest.raises(ConnectionRefusedError):
        await make_client()

    with pytest.raises(asyncio.TimeoutError):
        await asyncio.wait_for(
            asyncio.create_task(tcp_server.accept()),
            timeout=_NOTICEABLE_DELAY,
        )

    gate.start()
    tcp_client2 = await make_client()
    server_connection2 = await tcp_server.accept()
    await _assert_data_from_to(server_connection2, tcp_client2)
    await _assert_data_from_to(tcp_client2, server_connection2)
