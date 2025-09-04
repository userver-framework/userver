import asyncio
import socket
import time
import typing
import uuid

import pytest
from pytest_userver import chaos

# pylint: disable=import-error
from testsuite.asyncio_socket import AsyncioSocket
from testsuite.asyncio_socket import AsyncioSocketsFactory

_NOTICEABLE_DELAY = 0.5
TEST_TIMEOUT = 5
RECV_MAX_SIZE = 4096


async def _assert_receive_timeout(sock: AsyncioSocket) -> None:
    try:
        data = await sock.recv(RECV_MAX_SIZE, timeout=_NOTICEABLE_DELAY)
        assert not data
    except asyncio.TimeoutError:
        pass


class UdpServer:
    def __init__(self, sock: AsyncioSocket):
        self._sock = sock

    @property
    def sock(self):
        return self._sock

    @property
    def port(self):
        return self._sock.getsockname()[1]

    async def resolve(self, client_sock: AsyncioSocket):
        """
        Returns an address of the client as seen by the server
        """
        await client_sock.sendall(b'ping')
        msg, addr = await self._sock.recvfrom(RECV_MAX_SIZE)
        assert msg == b'ping'
        return addr


@pytest.fixture(name='udp_server', scope='function')
async def _server(make_socket):
    sock = make_socket()
    sock.bind(('127.0.0.1', 0))
    try:
        yield UdpServer(sock)
    finally:
        sock.close()


@pytest.fixture(name='gate', scope='function')
async def _gate(udp_server):
    gate_config = chaos.GateRoute(
        name='udp proxy',
        host_to_server='127.0.0.1',
        port_to_server=udp_server.port,
    )
    async with chaos.UdpGate(gate_config) as proxy:
        yield proxy


class Client(typing.NamedTuple):
    sock: AsyncioSocket
    addr_at_server: typing.Any


@pytest.fixture(name='make_socket')
def _make_socket(asyncio_socket: AsyncioSocketsFactory):
    def make_socket():
        return asyncio_socket.udp()

    return make_socket


@pytest.fixture(name='udp_client_factory')
async def _client_factory(gate, udp_server: UdpServer, make_socket):
    client_sockets: typing.List[socket.socket] = []

    async def _client_factory_impl():
        sock = make_socket()
        await sock.connect(gate.get_sockname_for_clients())
        client_sockets.append(sock)
        addr_at_server = await udp_server.resolve(sock)
        return Client(sock, addr_at_server)

    yield _client_factory_impl

    for sock in client_sockets:
        sock.close()


async def _assert_data_from_client(client: Client, server: UdpServer):
    expected = b'ping_' + uuid.uuid4().bytes
    await client.sock.sendall(expected)
    data, addr = await server.sock.recvfrom(len(expected))
    assert data == expected
    assert addr == client.addr_at_server


async def _assert_data_to_client(server: UdpServer, client: Client):
    expected = b'pong_' + uuid.uuid4().bytes
    await server.sock.sendto(expected, client.addr_at_server)
    data = await client.sock.recv(len(expected))
    assert data == expected


async def test_basic(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await _assert_data_from_client(udp_client, udp_server)
    await _assert_data_to_client(udp_server, udp_client)

    assert gate.is_connected()
    await gate.sockets_close()
    assert not gate.is_connected()


async def test_to_client_udp_message_queue(udp_server, udp_client_factory):
    udp_client = await udp_client_factory()

    messages_count = 5
    for i in range(messages_count):
        await udp_server.sock.sendto(
            b'message' + i.to_bytes(1, 'big'),
            udp_client.addr_at_server,
        )

    for i in range(messages_count):
        assert await udp_client.sock.recv(
            RECV_MAX_SIZE,
        ) == b'message' + i.to_bytes(1, 'big')


async def test_to_server_udp_message_queue(udp_server, udp_client_factory):
    udp_client = await udp_client_factory()

    messages_count = 5
    for i in range(messages_count):
        await udp_client.sock.sendall(b'message' + i.to_bytes(1, 'big'))

    for i in range(messages_count):
        assert await udp_server.sock.recv(
            RECV_MAX_SIZE,
        ) == b'message' + i.to_bytes(1, 'big')


async def test_fifo_udp_message_queue(udp_server, udp_client_factory):
    udp_client = await udp_client_factory()

    messages_count = 5
    for i in range(messages_count):
        await udp_client.sock.sendall(b'message' + i.to_bytes(1, 'big'))

    for i in range(messages_count):
        await udp_server.sock.sendto(
            b'message' + i.to_bytes(1, 'big'),
            udp_client.addr_at_server,
        )

    for i in range(messages_count):
        assert await udp_client.sock.recv(
            RECV_MAX_SIZE,
        ) == b'message' + i.to_bytes(1, 'big')

    for i in range(messages_count):
        assert await udp_server.sock.recv(
            RECV_MAX_SIZE,
        ) == b'message' + i.to_bytes(1, 'big')


async def test_to_server_parallel_udp_message(udp_server, udp_client_factory):
    udp_client = await udp_client_factory()

    messages_count = 100

    tasks: typing.List[typing.Awaitable] = []
    for i in range(messages_count):
        tasks.append(
            udp_client.sock.sendall(b'message' + i.to_bytes(1, 'big')),
        )

    await asyncio.gather(*tasks)

    res: typing.List[bool] = [False] * messages_count
    for _ in range(messages_count):
        message = await udp_server.sock.recv(RECV_MAX_SIZE)
        assert message[:7] == b'message'
        idx = int.from_bytes(message[7:], 'big')
        res[idx] = True

    assert all(res)


async def test_to_client_noop(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_client_noop()

    await udp_server.sock.sendto(b'ping', udp_client.addr_at_server)
    await _assert_data_from_client(udp_client, udp_server)
    assert not udp_client.sock.has_data()

    await gate.to_client_pass()
    hello = await udp_client.sock.recv(4)
    assert hello == b'ping'
    await _assert_data_to_client(udp_server, udp_client)
    await _assert_data_from_client(udp_client, udp_server)


async def test_to_server_noop(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_server_noop()

    await udp_client.sock.sendall(b'ping')
    await _assert_data_to_client(udp_server, udp_client)
    assert not udp_server.sock.has_data()

    await gate.to_server_pass()
    server_incoming_data = await udp_server.sock.recv(4)
    assert server_incoming_data == b'ping'
    await _assert_data_to_client(udp_server, udp_client)
    await _assert_data_from_client(udp_client, udp_server)


async def test_to_client_drop(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_client_drop()

    await udp_server.sock.sendto(b'ping', udp_client.addr_at_server)
    await _assert_data_from_client(udp_client, udp_server)
    assert not udp_client.sock.has_data()

    await gate.to_client_pass()
    await _assert_receive_timeout(udp_client.sock)


async def test_to_server_drop(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    drop_queue = await gate.to_server_drop()

    await udp_client.sock.sendall(b'ping')
    await _assert_data_to_client(udp_server, udp_client)
    assert not udp_server.sock.has_data()
    await drop_queue.wait_call()

    await gate.to_server_pass()
    await _assert_receive_timeout(udp_server.sock)


async def test_to_client_delay(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_client_delay(2 * _NOTICEABLE_DELAY)

    await _assert_data_from_client(udp_client, udp_server)

    start_time = time.monotonic()
    await _assert_data_to_client(udp_server, udp_client)
    assert time.monotonic() - start_time >= _NOTICEABLE_DELAY


async def test_to_server_delay(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_server_delay(2 * _NOTICEABLE_DELAY)

    await _assert_data_to_client(udp_server, udp_client)

    start_time = time.monotonic()
    await _assert_data_from_client(udp_client, udp_server)
    assert time.monotonic() - start_time >= _NOTICEABLE_DELAY


async def test_to_client_close_on_data(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_client_close_on_data()

    await _assert_data_from_client(udp_client, udp_server)
    assert gate.is_connected()
    await udp_server.sock.sendto(b'die', udp_client.addr_at_server)

    await _assert_receive_timeout(udp_client.sock)


async def test_to_server_close_on_data(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_server_close_on_data()

    await _assert_data_to_client(udp_server, udp_client)
    assert gate.is_connected()
    await udp_client.sock.sendall(b'die')

    await _assert_receive_timeout(udp_server.sock)


async def test_to_client_corrupt_data(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_client_corrupt_data()

    await _assert_data_from_client(udp_client, udp_server)
    assert gate.is_connected()

    await udp_server.sock.sendto(b'break me', udp_client.addr_at_server)
    data = await udp_client.sock.recv(512)
    assert data
    assert data != b'break me'

    await gate.to_client_pass()
    await _assert_data_to_client(udp_server, udp_client)
    await _assert_data_from_client(udp_client, udp_server)


async def test_to_server_corrupt_data(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_server_corrupt_data()

    await _assert_data_to_client(udp_server, udp_client)
    assert gate.is_connected()

    await udp_client.sock.sendall(b'break me')
    data = await udp_server.sock.recv(512)
    assert data
    assert data != b'break me'

    await gate.to_server_pass()
    await _assert_data_to_client(udp_server, udp_client)
    await _assert_data_from_client(udp_client, udp_server)


async def test_to_client_limit_bps(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_client_limit_bps(2)

    start_time = time.monotonic()

    message = b'hello'

    # udp sockets drops message ending, so we have only the part what
    # was read on first try
    for _ in range(5):
        await udp_server.sock.sendto(message, udp_client.addr_at_server)
        data = await udp_client.sock.recv(5)
        assert data
        assert data != message and message.startswith(data)

    assert time.monotonic() - start_time >= 1.0


async def test_to_server_limit_bps(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_server_limit_bps(2)

    start_time = time.monotonic()

    message = b'hello'

    # udp sockets drops message ending, so we have only the
    # part what was read on first try
    for _ in range(5):
        await udp_client.sock.sendall(b'hello')
        data = await udp_server.sock.recv(5)
        assert data
        assert len(data) and message.startswith(data)

    assert time.monotonic() - start_time >= 1.0


async def test_to_client_limit_time(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_client_limit_time(_NOTICEABLE_DELAY, jitter=0.0)

    await _assert_data_from_client(udp_client, udp_server)
    assert gate.is_connected()

    await asyncio.sleep(_NOTICEABLE_DELAY)

    await _assert_data_to_client(udp_server, udp_client)
    await asyncio.sleep(_NOTICEABLE_DELAY * 2)
    await udp_server.sock.sendto(b'die', udp_client.addr_at_server)
    await _assert_receive_timeout(udp_client.sock)


async def test_to_server_limit_time(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_server_limit_time(_NOTICEABLE_DELAY, jitter=0.0)

    await _assert_data_to_client(udp_server, udp_client)
    assert gate.is_connected()

    await asyncio.sleep(_NOTICEABLE_DELAY)

    await _assert_data_from_client(udp_client, udp_server)
    await asyncio.sleep(_NOTICEABLE_DELAY * 2)
    await udp_client.sock.sendall(b'die')
    await _assert_receive_timeout(udp_server.sock)


async def test_to_client_limit_bytes(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_client_limit_bytes(14)

    await _assert_data_from_client(udp_client, udp_server)
    assert gate.is_connected()

    await udp_server.sock.sendto(b'hello', udp_client.addr_at_server)
    data = await udp_client.sock.recv(10)
    assert data == b'hello'

    await udp_server.sock.sendto(b'die after', udp_client.addr_at_server)
    data = await udp_client.sock.recv(10)
    assert data == b'die after'

    await udp_server.sock.sendto(b'dead now', udp_client.addr_at_server)

    await _assert_receive_timeout(udp_client.sock)


async def test_to_server_limit_bytes(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_server_limit_bytes(14)

    await _assert_data_to_client(udp_server, udp_client)
    assert gate.is_connected()

    await udp_client.sock.sendall(b'hello')
    data = await udp_server.sock.recv(10)
    assert data == b'hello'

    await udp_client.sock.sendall(b'die after')
    data = await udp_server.sock.recv(10)
    assert data == b'die after'

    await _assert_receive_timeout(udp_server.sock)


async def test_substitute(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.to_server_substitute('hello', 'die')
    await gate.to_client_substitute('hello', 'die')

    await _assert_data_to_client(udp_server, udp_client)
    await _assert_data_from_client(udp_client, udp_server)

    await udp_client.sock.sendall(b'hello')
    data = await udp_server.sock.recv(10)
    assert data == b'die'

    await udp_server.sock.sendto(b'hello', udp_client.addr_at_server)
    data = await udp_client.sock.recv(10)
    assert data == b'die'


async def test_wait_for_connections(udp_client_factory, gate):
    await udp_client_factory()
    assert gate.is_connected()
    with pytest.raises(AttributeError):
        await gate.wait_for_connections(count=1, timeout=_NOTICEABLE_DELAY)


async def test_start_stop(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()

    await gate.stop()

    await udp_client.sock.sendall(b'hello')
    await _assert_receive_timeout(udp_server.sock)

    gate.start()
    udp_client2 = await udp_client_factory()

    await _assert_data_to_client(udp_server, udp_client2)
    await _assert_data_from_client(udp_client2, udp_server)


async def test_multi_client(udp_server, udp_client_factory, gate):
    udp_client = await udp_client_factory()
    udp_client2 = await udp_client_factory()

    await _assert_data_to_client(udp_server, udp_client)
    await _assert_data_to_client(udp_server, udp_client2)
    await _assert_data_from_client(udp_client2, udp_server)
    await _assert_data_from_client(udp_client, udp_server)
