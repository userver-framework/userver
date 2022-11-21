"""
Python module that provides testsuite support for
chaos tests; see @ref md_en_userver_chaos_testing for an introduction.

@ingroup userver_testsuite
"""

import asyncio
import dataclasses
import errno
import fcntl
import logging
import os
import random
import socket
import sys
import time
import typing


@dataclasses.dataclass(frozen=True)
class GateRoute:
    """
    Class that describes the route for TcpGate.

    Use `port_for_client == 0` to bind to some unused port. In that case the
    actual address could be retrieved via TcpGate.get_sockname_for_clients().

    @ingroup userver_testsuite
    """

    name: str
    host_to_server: str
    port_to_server: int
    host_for_client: str = 'localhost'
    port_for_client: int = 0


# @cond

# https://docs.python.org/3/library/socket.html#socket.socket.recv
RECV_MAX_SIZE = 1024 * 1024 * 128
MAX_DELAY = 60.0


logger = logging.getLogger(__name__)


EvLoop = typing.Any
Socket = socket.socket
Interceptor = typing.Callable[
    [EvLoop, Socket, Socket], typing.Coroutine[typing.Any, typing.Any, None],
]


class GateException(Exception):
    pass


class GateInterceptException(Exception):
    pass


async def _yield() -> None:
    _MIN_DELAY = 0.01
    await asyncio.sleep(_MIN_DELAY)


def _incomming_data_size(recv_socket: Socket) -> int:
    try:
        data = recv_socket.recv(RECV_MAX_SIZE, socket.MSG_PEEK)
        return len(data)
    except socket.error as e:
        err = e.args[0]
        if err in {errno.EAGAIN, errno.EWOULDBLOCK}:
            return 0
        raise e


async def _intercept_ok(
        loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    data = await loop.sock_recv(socket_from, RECV_MAX_SIZE)
    await loop.sock_sendall(socket_to, data)


async def _intercept_noop(
        loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    pass


async def _intercept_delay(
        delay: float, loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    data = await loop.sock_recv(socket_from, RECV_MAX_SIZE)
    await asyncio.sleep(delay)
    await loop.sock_sendall(socket_to, data)


async def _intercept_close_on_data(
        loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    await loop.sock_recv(socket_from, 1)
    raise GateInterceptException('Closing socket on data')


async def _intercept_corrupt(
        loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    data = await loop.sock_recv(socket_from, RECV_MAX_SIZE)
    await loop.sock_sendall(socket_to, bytearray([not x for x in data]))


class _InterceptBpsLimit:
    def __init__(self, bytes_per_second: float):
        assert bytes_per_second >= 1
        self._bytes_per_second = bytes_per_second
        self._time_last_added = 0.0
        self._bytes_left = self._bytes_per_second

    def _update_limit(self) -> None:
        current_time = time.monotonic()
        elapsed = current_time - self._time_last_added
        bytes_addition = self._bytes_per_second * elapsed
        if bytes_addition > 0:
            self._bytes_left += bytes_addition
            self._time_last_added = current_time

            if self._bytes_left > self._bytes_per_second:
                self._bytes_left = self._bytes_per_second

    async def __call__(
            self, loop: EvLoop, socket_from: Socket, socket_to: Socket,
    ) -> None:
        self._update_limit()

        bytes_to_recv = min(int(self._bytes_left), RECV_MAX_SIZE)
        if bytes_to_recv > 0:
            data = await loop.sock_recv(socket_from, bytes_to_recv)
            self._bytes_left -= len(data)

            await loop.sock_sendall(socket_to, data)
        else:
            logger.info('Socket hits the bytes per second limit')
            await asyncio.sleep(1.0 / self._bytes_per_second)


class _InterceptTimeLimit:
    def __init__(self, timeout: float, jitter: float):
        self._sockets: typing.Dict[Socket, float] = {}
        assert timeout >= 0.0
        self._timeout = timeout
        assert jitter >= 0.0
        self._jitter = jitter

    def raise_if_timed_out(self, socket_from: Socket) -> None:
        if socket_from not in self._sockets:
            jitter = self._jitter * random.random()
            expire_at = time.monotonic() + self._timeout + jitter
            self._sockets[socket_from] = expire_at

        if self._sockets[socket_from] <= time.monotonic():
            socket_from.close()
            del self._sockets[socket_from]
            raise GateInterceptException('Socket hits the time limit')

    async def __call__(
            self, loop: EvLoop, socket_from: Socket, socket_to: Socket,
    ) -> None:
        self.raise_if_timed_out(socket_from)
        await _intercept_ok(loop, socket_from, socket_to)


class _InterceptSmallerParts:
    def __init__(self, max_size: int):
        assert max_size > 0
        self._max_size = max_size

    async def __call__(
            self, loop: EvLoop, socket_from: Socket, socket_to: Socket,
    ) -> None:
        incomming_size = _incomming_data_size(socket_from)
        chunk_size = min(incomming_size, self._max_size)
        data = await loop.sock_recv(socket_from, chunk_size)
        await loop.sock_sendall(socket_to, data)


class _InterceptConcatPackets:
    def __init__(self, packet_size: int):
        assert packet_size >= 0
        self._packet_size = packet_size
        self._expire_at: typing.Optional[float] = None

    async def __call__(
            self, loop: EvLoop, socket_from: Socket, socket_to: Socket,
    ) -> None:
        if self._expire_at is None:
            self._expire_at = time.monotonic() + MAX_DELAY

        if self._expire_at <= time.monotonic():
            logger.error(
                f'Failed to make a packet of sufficient size in {MAX_DELAY} '
                'seconds. Check the test logic, it should end with checking '
                'that the data was sent and by calling TcpGate function '
                'to_client_pass() to pass the remaining packets.',
            )
            sys.exit(2)

        incomming_size = _incomming_data_size(socket_from)
        if incomming_size >= self._packet_size:
            data = await loop.sock_recv(socket_from, RECV_MAX_SIZE)
            await loop.sock_sendall(socket_to, data)
            self._expire_at = None


class _InterceptBytesLimit:
    def __init__(self, bytes_limit: int, gate: 'TcpGate'):
        assert bytes_limit >= 0
        self._bytes_limit = bytes_limit
        self._bytes_remain = self._bytes_limit
        self._gate = gate

    async def __call__(
            self, loop: EvLoop, socket_from: Socket, socket_to: Socket,
    ) -> None:
        data = await loop.sock_recv(socket_from, RECV_MAX_SIZE)
        if self._bytes_remain <= len(data):
            await loop.sock_sendall(socket_to, data[0 : self._bytes_remain])
            await self._gate.sockets_close()
            raise GateInterceptException('Data transmission limit reached')

        self._bytes_remain -= len(data)
        await loop.sock_sendall(socket_to, data)


async def _cancel_and_join(task: typing.Optional[asyncio.Task]) -> None:
    if not task or task.cancelled():
        return

    try:
        task.cancel()
        await task
    except asyncio.CancelledError:
        return
    except Exception as exc:  # pylint: disable=broad-except
        logger.warning('Exception in _cancel_and_join: %s', exc)


class _SocketsPaired:
    def __init__(
            self,
            route: GateRoute,
            loop: EvLoop,
            client: socket.socket,
            right: socket.socket,
            to_server_intercept: Interceptor,
            to_client_intercept: Interceptor,
    ) -> None:
        self._route = route
        self._loop = loop

        self._client = client
        self._right = right

        self._to_server_intercept: Interceptor = to_server_intercept
        self._to_client_intercept: Interceptor = to_client_intercept

        self._tasks: typing.Optional[asyncio.Task] = asyncio.create_task(
            self._do_stream(),
        )

    async def _do_stream(self) -> None:
        await asyncio.wait(
            [
                asyncio.create_task(self._do_pipe_channels(to_server=True)),
                asyncio.create_task(self._do_pipe_channels(to_server=False)),
            ],
            return_when=asyncio.ALL_COMPLETED,
        )

    async def _do_pipe_channels(self, *, to_server: bool) -> None:
        if to_server:
            socket_from = self._client
            socket_to = self._right
        else:
            socket_from = self._right
            socket_to = self._client

        try:
            while True:
                # Applies new inreceptors faster.
                #
                # To avoid long awaiting on sock_recv in an outdated
                # interceptor we wait for data before grabbing and applying
                # the interceptor.
                if not _incomming_data_size(socket_from):
                    await _yield()
                    continue

                if to_server:
                    interceptor = self._to_server_intercept
                else:
                    interceptor = self._to_client_intercept

                await interceptor(self._loop, socket_from, socket_to)
                await _yield()
        except Exception as exc:  # pylint: disable=broad-except
            logger.info('Exception in "%s": %s', self._route.name, exc)
        finally:
            self._close_sockets()

    def set_to_server_interceptor(self, interceptor: Interceptor) -> None:
        self._to_server_intercept = interceptor

    def set_to_client_interceptor(self, interceptor: Interceptor) -> None:
        self._to_client_intercept = interceptor

    def _close_sockets(self) -> None:
        try:
            self._client.close()
        except Exception as exc:  # pylint: disable=broad-except
            logger.warning(
                'Exception in "%s" on closing client: %s',
                self._route.name,
                exc,
            )

        try:
            self._right.close()
        except Exception as exc:  # pylint: disable=broad-except
            logger.warning(
                'Exception in "%s" on closing right: %s',
                self._route.name,
                exc,
            )

    async def shutdown(self) -> None:
        self._close_sockets()
        await _cancel_and_join(self._tasks)
        self._tasks = None

    def is_active(self) -> bool:
        return bool(self._tasks)


# @endcond


class TcpGate:
    """
    Accepts incoming client connections (host_for_client, port_for_client),
    on each new connection on client connects to server
    (host_to_server, port_to_server).

    Asynchronously concurrently passes data from client to server and from
    server to client, allowing intercepting the data, injecting delays and
    dropping connections.

    @ingroup userver_testsuite
    """

    def __init__(self, route: GateRoute, loop) -> None:
        self._route = route
        self._loop = loop

        self._to_server_intercept: Interceptor = _intercept_ok
        self._to_client_intercept: Interceptor = _intercept_ok

        self._accept_sock: typing.Optional[socket.socket] = None
        self._accept_task: typing.Optional[asyncio.Task[None]] = None

        self._connected_event = asyncio.Event()

        self._sockets: typing.Set[_SocketsPaired] = set()

    async def __aenter__(self) -> 'TcpGate':
        self.start()
        return self

    async def __aexit__(self, exc_type, exc_value, traceback) -> None:
        await self.stop()
        await _yield()

    def start(self) -> None:
        """ Open the socket and start accepting connections from client """
        if self._accept_sock:
            return

        self._accept_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._accept_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        client_addr = (
            self._route.host_for_client,
            self._route.port_for_client,
        )
        self._accept_sock.bind(client_addr)

        if self._route.port_for_client == 0:
            # In case of stop()+start() bind to the same port
            self._route = GateRoute(
                name=self._route.name,
                host_to_server=self._route.host_to_server,
                port_to_server=self._route.port_to_server,
                host_for_client=self._accept_sock.getsockname()[0],
                port_for_client=self._accept_sock.getsockname()[1],
            )

        self._accept_sock.listen()

        self.start_accepting()

    def start_accepting(self) -> None:
        """ Start accepting incoming connections from client """
        assert self._accept_sock
        if self._accept_task and not self._accept_task.done():
            return

        self._accept_task = asyncio.create_task(self._do_accept())

    async def stop_accepting(self) -> None:
        """
        Stop accepting incoming connections from client without closing
        the accepting socket.
        """
        await _cancel_and_join(self._accept_task)
        self._accept_task = None

    async def stop(self) -> None:
        """
        Stop accepting incoming connections from client, close all the sockets
        and connections
        """
        if not self._accept_sock:
            return

        self.to_server_pass()
        self.to_client_pass()

        self._accept_sock.close()

        await _cancel_and_join(self._accept_task)
        await self.sockets_close()
        self._accept_sock = None

    def set_to_server_interceptor(self, interceptor: Interceptor) -> None:
        """
        Replace existing interceptor of client to server data with a custom
        """
        self._to_server_intercept = interceptor
        for x in self._sockets:
            x.set_to_server_interceptor(self._to_server_intercept)

    def set_to_client_interceptor(self, interceptor: Interceptor) -> None:
        """
        Replace existing interceptor of server to client data with a custom
        """
        self._to_client_intercept = interceptor
        for x in self._sockets:
            x.set_to_client_interceptor(self._to_client_intercept)

    def to_server_pass(self) -> None:
        """ Pass data as is """
        self.set_to_server_interceptor(_intercept_ok)

    def to_client_pass(self) -> None:
        """ Pass data as is """
        self.set_to_client_interceptor(_intercept_ok)

    def to_server_noop(self) -> None:
        """ Do not read data, causing client to keep multiple data """
        self.set_to_server_interceptor(_intercept_noop)

    def to_client_noop(self) -> None:
        """ Do not read data, causing server to keep multiple data """
        self.set_to_client_interceptor(_intercept_noop)

    def to_server_delay(self, delay: float) -> None:
        """ Delay data transmission """

        async def _intercept_delay_bound(
                loop: EvLoop, socket_from: Socket, socket_to: Socket,
        ) -> None:
            await _intercept_delay(delay, loop, socket_from, socket_to)

        self.set_to_server_interceptor(_intercept_delay_bound)

    def to_client_delay(self, delay: float) -> None:
        """ Delay data transmission """

        async def _intercept_delay_bound(
                loop: EvLoop, socket_from: Socket, socket_to: Socket,
        ) -> None:
            await _intercept_delay(delay, loop, socket_from, socket_to)

        self.set_to_client_interceptor(_intercept_delay_bound)

    def to_server_close_on_data(self) -> None:
        """ Close on first bytes of data from client """
        self.set_to_server_interceptor(_intercept_close_on_data)

    def to_client_close_on_data(self) -> None:
        """ Close on first bytes of data from server """
        self.set_to_client_interceptor(_intercept_close_on_data)

    def to_server_corrupt_data(self) -> None:
        """ Corrupt data received from client """
        self.set_to_server_interceptor(_intercept_corrupt)

    def to_client_corrupt_data(self) -> None:
        """ Corrupt data received from server """
        self.set_to_client_interceptor(_intercept_corrupt)

    def to_server_limit_bps(self, bytes_per_second: float) -> None:
        """ Limit bytes per second transmission by network from client """
        self.set_to_server_interceptor(_InterceptBpsLimit(bytes_per_second))

    def to_client_limit_bps(self, bytes_per_second: float) -> None:
        """ Limit bytes per second transmission by network from server """
        self.set_to_client_interceptor(_InterceptBpsLimit(bytes_per_second))

    def to_server_limit_time(self, timeout: float, jitter: float) -> None:
        """ Limit connection lifetime on receive of first bytes from client """
        self.set_to_server_interceptor(_InterceptTimeLimit(timeout, jitter))

    def to_client_limit_time(self, timeout: float, jitter: float) -> None:
        """ Limit connection lifetime on receive of first bytes from server """
        self.set_to_client_interceptor(_InterceptTimeLimit(timeout, jitter))

    def to_server_smaller_parts(self, max_size: int) -> None:
        """
        Pass data to server in smaller parts

        @param max_size Max packet size to send to server
        """
        self.set_to_server_interceptor(_InterceptSmallerParts(max_size))

    def to_client_smaller_parts(self, max_size: int) -> None:
        """
        Pass data to client in smaller parts

        @param max_size Max packet size to send to client
        """
        self.set_to_client_interceptor(_InterceptSmallerParts(max_size))

    def to_server_concat_packets(self, packet_size: int) -> None:
        """
        Pass data in bigger parts
        @param packet_size minimal size of the resulting packet
        """

        self.set_to_server_interceptor(_InterceptConcatPackets(packet_size))

    def to_client_concat_packets(self, packet_size: int) -> None:
        """
        Pass data in bigger parts
        @param packet_size minimal size of the resulting packet
        """

        self.set_to_client_interceptor(_InterceptConcatPackets(packet_size))

    def to_server_limit_bytes(self, bytes_limit: int) -> None:
        """ Drop all connections each `bytes_limit` of data sent by network """
        self.set_to_server_interceptor(_InterceptBytesLimit(bytes_limit, self))

    def to_client_limit_bytes(self, bytes_limit: int) -> None:
        """ Drop all connections each `bytes_limit` of data sent by network """
        self.set_to_client_interceptor(_InterceptBytesLimit(bytes_limit, self))

    def connections_count(self) -> int:
        """
        Returns maximal amount of connections going through the gate at
        the moment. Some of the connection could be closing, use the value
        with caution.
        """
        return len(self._sockets)

    async def sockets_close(
            self, *, count: typing.Optional[int] = None,
    ) -> None:
        """ Close all the connection going through the gate """
        for x in list(self._sockets)[0:count]:
            await x.shutdown()
        self._collect_garbage()

    async def wait_for_connections(self, *, count=1, timeout=0.0) -> None:
        """
        Wait for at least `count` connections going through the gate.

        @throws asyncio.TimeoutError exception if failed to get the
                required amount of connections in time.
        """
        if timeout <= 0.0:
            while self.connections_count() < count:
                await self._connected_event.wait()
                self._connected_event.clear()
            return

        deadline = time.monotonic() + timeout
        while self.connections_count() < count:
            time_left = deadline - time.monotonic()
            await asyncio.wait_for(
                self._connected_event.wait(), timeout=time_left,
            )
            self._connected_event.clear()

    def get_sockname_for_clients(self) -> typing.Tuple[str, int]:
        """
        Returns the client socket address that the gate listens on.

        This function allows to use 0 in GateRoute.port_for_client and retrieve
        the actual port and host.
        """
        assert self._route.port_for_client != 0, (
            'Gate was not started and the port_for_client is still 0',
        )
        return (self._route.host_for_client, self._route.port_for_client)

    def _collect_garbage(self) -> None:
        self._sockets = {x for x in self._sockets if x.is_active()}

    async def _do_accept(self) -> None:
        while self._accept_sock:
            client, _ = await self._loop.sock_accept(self._accept_sock)
            client.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            fcntl.fcntl(client, fcntl.F_SETFL, os.O_NONBLOCK)

            server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            dest = (self._route.host_to_server, self._route.port_to_server)
            try:
                await self._loop.sock_connect(server, dest)
                server.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                fcntl.fcntl(server, fcntl.F_SETFL, os.O_NONBLOCK)
                self._sockets.add(
                    _SocketsPaired(
                        self._route,
                        self._loop,
                        client,
                        server,
                        self._to_server_intercept,
                        self._to_client_intercept,
                    ),
                )
                self._connected_event.set()
            except Exception as exc:  # pylint: disable=broad-except
                logger.warning(
                    'Exception in "%s" on piping %s: %s',
                    self._route.name,
                    dest,
                    exc,
                )
                client.close()
                server.close()

            self._collect_garbage()
