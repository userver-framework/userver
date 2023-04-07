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
import re
import socket
import sys
import time
import typing


@dataclasses.dataclass(frozen=True)
class GateRoute:
    """
    Class that describes the route for TcpGate or UdpGate.

    Use `port_for_client == 0` to bind to some unused port. In that case the
    actual address could be retrieved via BaseGate.get_sockname_for_clients().

    @ingroup userver_testsuite
    """

    name: str
    host_to_server: str
    port_to_server: int
    host_for_client: str = '127.0.0.1'
    port_for_client: int = 0


# @cond

# https://docs.python.org/3/library/socket.html#socket.socket.recv
RECV_MAX_SIZE = 4096
MAX_DELAY = 60.0


logger = logging.getLogger(__name__)


Address = typing.Tuple[str, int]
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
    # Minamal delay can be 0. This will be fast path for coroutine switching
    # https://docs.python.org/3/library/asyncio-task.html#sleeping

    _MIN_DELAY = 0
    await asyncio.sleep(_MIN_DELAY)


def _try_get_message(
        recv_socket: Socket,
) -> typing.Tuple[typing.Optional[bytes], typing.Optional[Address]]:

    try:
        return recv_socket.recvfrom(RECV_MAX_SIZE, socket.MSG_PEEK)
    except socket.error as e:
        err = e.args[0]
        if err in {errno.EAGAIN, errno.EWOULDBLOCK}:
            return None, None
        raise e


async def _wait_for_message_task(
        recv_socket: Socket,
) -> typing.Tuple[bytes, Address]:
    iteration = 0
    _LOG_FREQUENCY = 100
    while True:
        if not iteration % _LOG_FREQUENCY:
            logger.debug(
                f'Wait for message on socket fd={recv_socket.fileno()}. '
                f'Iteration {iteration}',
            )
        iteration += 1

        msg, addr = _try_get_message(recv_socket)
        if msg:
            assert addr
            return msg, addr

        await _yield()


def _incoming_data_size(recv_socket: Socket) -> int:
    msg, _ = _try_get_message(recv_socket)
    return len(msg) if msg else 0


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
        incoming_size = _incoming_data_size(socket_from)
        chunk_size = min(incoming_size, self._max_size)
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

        incoming_size = _incoming_data_size(socket_from)
        if incoming_size >= self._packet_size:
            data = await loop.sock_recv(socket_from, RECV_MAX_SIZE)
            await loop.sock_sendall(socket_to, data)
            self._expire_at = None


class _InterceptBytesLimit:
    def __init__(self, bytes_limit: int, gate: 'BaseGate'):
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
            self._bytes_remain = self._bytes_limit
            raise GateInterceptException('Data transmission limit reached')

        self._bytes_remain -= len(data)
        await loop.sock_sendall(socket_to, data)


class _InterceptSubstitute:
    def __init__(self, pattern: str, repl: str, encoding='utf-8'):
        self._pattern = re.compile(pattern)
        self._repl = repl
        self._encoding = encoding

    async def __call__(
            self, loop: EvLoop, socket_from: Socket, socket_to: Socket,
    ) -> None:
        data = await loop.sock_recv(socket_from, RECV_MAX_SIZE)
        try:
            res = self._pattern.sub(self._repl, data.decode(self._encoding))
            data = res.encode(self._encoding)
        except UnicodeError:
            pass
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
        logger.error('Exception in _cancel_and_join: %s', exc)


class _SocketsPaired:
    def __init__(
            self,
            proxy_name: str,
            loop: EvLoop,
            client: socket.socket,
            server: socket.socket,
            to_server_intercept: Interceptor,
            to_client_intercept: Interceptor,
    ) -> None:
        self._proxy_name = proxy_name
        self._loop = loop

        self._client = client
        self._server = server

        self._to_server_intercept: Interceptor = to_server_intercept
        self._to_client_intercept: Interceptor = to_client_intercept

        self._task_to_server = asyncio.create_task(
            self._do_pipe_channels(to_server=True),
        )
        self._task_to_client = asyncio.create_task(
            self._do_pipe_channels(to_server=False),
        )

        self._finished_channels = 0

    async def _do_pipe_channels(self, *, to_server: bool) -> None:
        if to_server:
            socket_from = self._client
            socket_to = self._server
        else:
            socket_from = self._server
            socket_to = self._client

        try:
            while True:
                # Applies new interceptors faster.
                #
                # To avoid long awaiting on sock_recv in an outdated
                # interceptor we wait for data before grabbing and applying
                # the interceptor.
                if not _incoming_data_size(socket_from):
                    await _yield()
                    continue

                if to_server:
                    interceptor = self._to_server_intercept
                else:
                    interceptor = self._to_client_intercept

                await interceptor(self._loop, socket_from, socket_to)
                await _yield()
        except GateInterceptException as exc:
            logger.info('In "%s": %s', self._proxy_name, exc)
        except socket.error as exc:
            logger.error('Exception in "%s": %s', self._proxy_name, exc)
        finally:
            self._finished_channels += 1
            if self._finished_channels == 2:
                # Closing the sockets here so that the self.shutdown()
                # returns only when the sockets are actually closed
                logger.info('"%s" closes  %s', self._proxy_name, self.info())
                self._close_socket(self._client)
                self._close_socket(self._server)
            else:
                assert self._finished_channels == 1
                if to_server:
                    self._task_to_client.cancel()
                else:
                    self._task_to_server.cancel()

    def set_to_server_interceptor(self, interceptor: Interceptor) -> None:
        self._to_server_intercept = interceptor

    def set_to_client_interceptor(self, interceptor: Interceptor) -> None:
        self._to_client_intercept = interceptor

    def _close_socket(self, self_socket: Socket) -> None:
        assert self_socket in {self._client, self._server}
        try:
            self_socket.close()
        except socket.error as exc:
            logger.error(
                'Exception in "%s" on closing %s: %s',
                self._proxy_name,
                'client' if self_socket == self._client else 'server',
                exc,
            )

    async def shutdown(self) -> None:
        for task in {self._task_to_client, self._task_to_server}:
            await _cancel_and_join(task)

    def is_active(self) -> bool:
        return (
            not self._task_to_client.done() or not self._task_to_server.done()
        )

    def info(self) -> str:
        if not self.is_active():
            return '<inactive>'

        return (
            f'client fd={self._client.fileno()} <=> '
            f'server fd={self._server.fileno()}'
        )


# @endcond


class BaseGate:
    """
    This base class maintain endpoints of two types:

    Server-side endpoints to receive messages from clients. Address of this
    endpoint is described by (host_for_client, port_for_client).

    Client-side endpoints to forward messages to server. Server must listen on
    (host_to_server, port_to_server).

    Asynchronously concurrently passes data from client to server and from
    server to client, allowing intercepting the data, injecting delays and
    dropping connections.

    @warning Do not use this class itself. Use one of the specifications
    TcpGate or UdpGate

    @ingroup userver_testsuite

    @see @ref md_en_userver_chaos_testing
    """

    _NOT_IMPLEMENTED_MESSAGE = (
        'Do not use BaseGate itself, use one of '
        'specializations TcpGate or UdpGate'
    )

    def __init__(self, route: GateRoute, loop: EvLoop) -> None:
        self._route = route
        self._loop = loop

        self._to_server_intercept: Interceptor = _intercept_ok
        self._to_client_intercept: Interceptor = _intercept_ok

        self._accept_sockets: typing.List[socket.socket] = []
        self._accept_tasks: typing.List[asyncio.Task[None]] = []

        self._connected_event = asyncio.Event()

        self._sockets: typing.Set[_SocketsPaired] = set()

    async def __aenter__(self) -> 'BaseGate':
        self.start()
        return self

    async def __aexit__(self, exc_type, exc_value, traceback) -> None:
        await self.stop()

    def _create_accepting_sockets(self) -> typing.List[Socket]:
        raise NotImplementedError(self._NOT_IMPLEMENTED_MESSAGE)

    def start(self):
        """ Open the socket and start accepting tasks """
        if self._accept_sockets:
            return

        self._accept_sockets.extend(self._create_accepting_sockets())

        if not self._accept_sockets:
            raise GateException(
                f'Could not resolve hostname {self._route.host_for_client}',
            )

        if self._route.port_for_client == 0:
            # In case of stop()+start() bind to the same port
            self._route = GateRoute(
                name=self._route.name,
                host_to_server=self._route.host_to_server,
                port_to_server=self._route.port_to_server,
                host_for_client=self._accept_sockets[0].getsockname()[0],
                port_for_client=self._accept_sockets[0].getsockname()[1],
            )

        BaseGate.start_accepting(self)

    def start_accepting(self) -> None:
        """ Start accepting tasks """
        assert self._accept_sockets
        if not all(tsk.done() for tsk in self._accept_tasks):
            return

        self._accept_tasks.clear()
        for sock in self._accept_sockets:
            self._accept_tasks.append(
                asyncio.create_task(self._do_accept(sock)),
            )

    async def stop_accepting(self) -> None:
        """
        Stop accepting tasks without closing the accepting socket.
        """
        for tsk in self._accept_tasks:
            await _cancel_and_join(tsk)
        self._accept_tasks.clear()

    async def stop(self) -> None:
        """
        Stop accepting tasks, close all the sockets
        """
        if not self._accept_sockets and not self._sockets:
            return

        self.to_server_pass()
        self.to_client_pass()

        for sock in self._accept_sockets:
            sock.close()

        await BaseGate.stop_accepting(self)
        logger.info('Before close() %s', self.info())
        await self.sockets_close()
        assert not self._sockets

        self._accept_sockets.clear()
        logger.info('Stopped. %s', self.info())

    async def sockets_close(
            self, *, count: typing.Optional[int] = None,
    ) -> None:
        """ Close all the connection going through the gate """
        for x in list(self._sockets)[0:count]:
            await x.shutdown()
        self._collect_garbage()

    def get_sockname_for_clients(self) -> Address:
        """
        Returns the client socket address that the gate listens on.

        This function allows to use 0 in GateRoute.port_for_client and retrieve
        the actual port and host.
        """
        assert self._route.port_for_client != 0, (
            'Gate was not started and the port_for_client is still 0',
        )
        return (self._route.host_for_client, self._route.port_for_client)

    def info(self) -> str:
        """ Print info on open sockets """
        if not self._sockets:
            return f'"{self._route.name}" no active sockets'

        return f'"{self._route.name}" active sockets:\n\t' + '\n\t'.join(
            x.info() for x in self._sockets
        )

    def _collect_garbage(self) -> None:
        self._sockets = {x for x in self._sockets if x.is_active()}

    async def _do_accept(self, accept_sock: Socket) -> None:
        """
        This task should wait for connections and create SocketPair
        """
        raise NotImplementedError(self._NOT_IMPLEMENTED_MESSAGE)

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
        logging.debug('to_server_pass')
        self.set_to_server_interceptor(_intercept_ok)

    def to_client_pass(self) -> None:
        """ Pass data as is """
        logging.debug('to_client_pass')
        self.set_to_client_interceptor(_intercept_ok)

    def to_server_noop(self) -> None:
        """ Do not read data, causing client to keep multiple data """
        logging.debug('to_server_noop')
        self.set_to_server_interceptor(_intercept_noop)

    def to_client_noop(self) -> None:
        """ Do not read data, causing server to keep multiple data """
        logging.debug('to_client_noop')
        self.set_to_client_interceptor(_intercept_noop)

    def to_server_delay(self, delay: float) -> None:
        """ Delay data transmission """
        logging.debug('to_server_delay, delay: %s', delay)

        async def _intercept_delay_bound(
                loop: EvLoop, socket_from: Socket, socket_to: Socket,
        ) -> None:
            await _intercept_delay(delay, loop, socket_from, socket_to)

        self.set_to_server_interceptor(_intercept_delay_bound)

    def to_client_delay(self, delay: float) -> None:
        """ Delay data transmission """
        logging.debug('to_client_delay, delay: %s', delay)

        async def _intercept_delay_bound(
                loop: EvLoop, socket_from: Socket, socket_to: Socket,
        ) -> None:
            await _intercept_delay(delay, loop, socket_from, socket_to)

        self.set_to_client_interceptor(_intercept_delay_bound)

    def to_server_close_on_data(self) -> None:
        """ Close on first bytes of data from client """
        logging.debug('to_server_close_on_data')
        self.set_to_server_interceptor(_intercept_close_on_data)

    def to_client_close_on_data(self) -> None:
        """ Close on first bytes of data from server """
        logging.debug('to_client_close_on_data')
        self.set_to_client_interceptor(_intercept_close_on_data)

    def to_server_corrupt_data(self) -> None:
        """ Corrupt data received from client """
        logging.debug('to_server_corrupt_data')
        self.set_to_server_interceptor(_intercept_corrupt)

    def to_client_corrupt_data(self) -> None:
        """ Corrupt data received from server """
        logging.debug('to_client_corrupt_data')
        self.set_to_client_interceptor(_intercept_corrupt)

    def to_server_limit_bps(self, bytes_per_second: float) -> None:
        """ Limit bytes per second transmission by network from client """
        logging.debug(
            'to_server_limit_bps, bytes_per_second: %s', bytes_per_second,
        )
        self.set_to_server_interceptor(_InterceptBpsLimit(bytes_per_second))

    def to_client_limit_bps(self, bytes_per_second: float) -> None:
        """ Limit bytes per second transmission by network from server """
        logging.debug(
            'to_client_limit_bps, bytes_per_second: %s', bytes_per_second,
        )
        self.set_to_client_interceptor(_InterceptBpsLimit(bytes_per_second))

    def to_server_limit_time(self, timeout: float, jitter: float) -> None:
        """ Limit connection lifetime on receive of first bytes from client """
        logging.debug(
            'to_server_limit_time, timeout: %s, jitter: %s', timeout, jitter,
        )
        self.set_to_server_interceptor(_InterceptTimeLimit(timeout, jitter))

    def to_client_limit_time(self, timeout: float, jitter: float) -> None:
        """ Limit connection lifetime on receive of first bytes from server """
        logging.debug(
            'to_client_limit_time, timeout: %s, jitter: %s', timeout, jitter,
        )
        self.set_to_client_interceptor(_InterceptTimeLimit(timeout, jitter))

    def to_server_smaller_parts(self, max_size: int) -> None:
        """
        Pass data to server in smaller parts

        @param max_size Max packet size to send to server
        """
        logging.debug('to_server_smaller_parts, max_size: %s', max_size)
        self.set_to_server_interceptor(_InterceptSmallerParts(max_size))

    def to_client_smaller_parts(self, max_size: int) -> None:
        """
        Pass data to client in smaller parts

        @param max_size Max packet size to send to client
        """
        logging.debug('to_client_smaller_parts, max_size: %s', max_size)
        self.set_to_client_interceptor(_InterceptSmallerParts(max_size))

    def to_server_concat_packets(self, packet_size: int) -> None:
        """
        Pass data in bigger parts
        @param packet_size minimal size of the resulting packet
        """
        logging.debug('to_server_concat_packets, packet_size: %s', packet_size)
        self.set_to_server_interceptor(_InterceptConcatPackets(packet_size))

    def to_client_concat_packets(self, packet_size: int) -> None:
        """
        Pass data in bigger parts
        @param packet_size minimal size of the resulting packet
        """
        logging.debug('to_client_concat_packets, packet_size: %s', packet_size)
        self.set_to_client_interceptor(_InterceptConcatPackets(packet_size))

    def to_server_limit_bytes(self, bytes_limit: int) -> None:
        """ Drop all connections each `bytes_limit` of data sent by network """
        logging.debug('to_server_limit_bytes, bytes_limit: %s', bytes_limit)
        self.set_to_server_interceptor(_InterceptBytesLimit(bytes_limit, self))

    def to_client_limit_bytes(self, bytes_limit: int) -> None:
        """ Drop all connections each `bytes_limit` of data sent by network """
        logging.debug('to_client_limit_bytes, bytes_limit: %s', bytes_limit)
        self.set_to_client_interceptor(_InterceptBytesLimit(bytes_limit, self))

    def to_server_substitute(self, pattern: str, repl: str) -> None:
        """ Apply regex substitution to data from client """
        logging.debug(
            'to_server_substitute, pattern: %s, repl: %s', pattern, repl,
        )
        self.set_to_server_interceptor(_InterceptSubstitute(pattern, repl))

    def to_client_substitute(self, pattern: str, repl: str) -> None:
        """ Apply regex substitution to data from server """
        logging.debug(
            'to_client_substitute, pattern: %s, repl: %s', pattern, repl,
        )
        self.set_to_client_interceptor(_InterceptSubstitute(pattern, repl))


class TcpGate(BaseGate):
    """
    Implements TCP chaos-proxy logic such as accepting incoming tcp client
    connections. On each new connection new tcp client connects to server
    (host_to_server, port_to_server).

    @ingroup userver_testsuite

    @see @ref md_en_userver_chaos_testing
    """

    def __init__(self, route: GateRoute, loop: EvLoop) -> None:
        BaseGate.__init__(self, route, loop)

    def connections_count(self) -> int:
        """
        Returns maximal amount of connections going through the gate at
        the moment.

        @warning Some of the connections could be closing, or could be opened
                 right before the function starts. Use with caution!
        """
        return len(self._sockets)

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

    def _make_socket_nonblocking(self, sock: Socket) -> None:
        sock.setblocking(False)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        fcntl.fcntl(sock, fcntl.F_SETFL, os.O_NONBLOCK)

    def _create_accepting_sockets(self) -> typing.List[Socket]:
        res: typing.List[Socket] = []
        for addr in socket.getaddrinfo(
                self._route.host_for_client,
                self._route.port_for_client,
                type=socket.SOCK_STREAM,
        ):
            sock = Socket(addr[0], addr[1])
            self._make_socket_nonblocking(sock)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.bind(addr[4])
            sock.listen()
            logger.debug(
                f'Accepting connections on {sock.getsockname()}, '
                f'fd={sock.fileno()}',
            )
            res.append(sock)

        return res

    async def _connect_to_server(self):
        addrs = await self._loop.getaddrinfo(
            self._route.host_to_server,
            self._route.port_to_server,
            type=socket.SOCK_STREAM,
        )
        for addr in addrs:
            server = Socket(addr[0], addr[1])
            try:
                self._make_socket_nonblocking(server)
                await self._loop.sock_connect(server, addr[4])
                logging.debug('Connected to %s', addr[4])
                return server
            except Exception as exc:  # pylint: disable=broad-except
                server.close()
                logging.warning('Could not connect to %s: %s', addr[4], exc)

    async def _do_accept(self, accept_sock: Socket) -> None:
        while accept_sock:
            client, _ = await self._loop.sock_accept(accept_sock)
            self._make_socket_nonblocking(client)

            server = await self._connect_to_server()
            if server:
                self._sockets.add(
                    _SocketsPaired(
                        self._route.name,
                        self._loop,
                        client,
                        server,
                        self._to_server_intercept,
                        self._to_client_intercept,
                    ),
                )
                self._connected_event.set()
            else:
                client.close()

            self._collect_garbage()


class UdpGate(BaseGate):
    """
    Implements UDP chaos-proxy logic such as waiting for first
    message and setting up sockets for forwarding messages between
    udp-client and udp-server

    @ingroup userver_testsuite

    @see @ref md_en_userver_chaos_testing
    """

    _NOT_IMPLEMENTED_IN_UDP = 'This method is not allowed in UDP gate'

    def __init__(self, route: GateRoute, loop: EvLoop):
        self._client_addr: typing.Optional[Address] = None
        BaseGate.__init__(self, route, loop)

    def is_connected(self) -> bool:
        """
        Returns True if there is active pair of sockets ready to transfer data
        at the moment.
        """
        return len(self._sockets) > 0

    def _make_socket_nonblocking(self, sock: Socket) -> None:
        sock.setblocking(False)
        fcntl.fcntl(sock, fcntl.F_SETFL, os.O_NONBLOCK)

    def _create_accepting_sockets(self) -> typing.List[Socket]:
        res: typing.List[Socket] = []
        for addr in socket.getaddrinfo(
                self._route.host_for_client,
                self._route.port_for_client,
                type=socket.SOCK_DGRAM,
        ):
            sock = socket.socket(addr[0], addr[1])
            self._make_socket_nonblocking(sock)
            sock.bind(addr[4])
            logger.debug(f'Accepting connections on {sock.getsockname()}')
            res.append(sock)

        return res

    async def _connect_to_server(self):
        addrs = await self._loop.getaddrinfo(
            self._route.host_to_server,
            self._route.port_to_server,
            type=socket.SOCK_DGRAM,
        )
        for addr in addrs:
            server = Socket(addr[0], addr[1])
            try:
                self._make_socket_nonblocking(server)
                await self._loop.sock_connect(server, addr[4])
                logging.debug('Connected to %s', addr[4])
                return server
            except Exception as exc:  # pylint: disable=broad-except
                logging.warning('Could not connect to %s: %s', addr[4], exc)

    async def _do_accept(self, accept_sock: Socket):
        if not accept_sock:
            return

        _, addr = await _wait_for_message_task(accept_sock)

        self._client_addr = addr
        try:
            await self._loop.sock_connect(accept_sock, addr)
        except Exception as exc:  # pylint: disable=broad-except
            logging.warning('Could not connect to %s: %s', addr, exc)

        server = await self._connect_to_server()
        if server:
            self._sockets.add(
                _SocketsPaired(
                    self._route.name,
                    self._loop,
                    accept_sock,
                    server,
                    self._to_server_intercept,
                    self._to_client_intercept,
                ),
            )
            self._connected_event.set()
        else:
            accept_sock.close()

        self._collect_garbage()

    def start_accepting(self) -> None:
        raise NotImplementedError(
            'Since UdpGate can only have one connection, you cannot start or '
            'stop accepting tasks manually. Use start() and stop() methods to '
            'stop data transferring',
        )

    async def stop_accepting(self) -> None:
        raise NotImplementedError(
            'Since UdpGate can only have one connection, you cannot start or '
            'stop accepting tasks manually. Use start() and stop() methods to '
            'stop data transferring',
        )

    def to_server_concat_packets(self, packet_size: int) -> None:
        raise NotImplementedError('Udp packets cannot be concatenated')

    def to_client_concat_packets(self, packet_size: int) -> None:
        raise NotImplementedError('Udp packets cannot be concatenated')

    def to_server_smaller_parts(self, max_size: int) -> None:
        raise NotImplementedError('Udp packets cannot be split')

    def to_client_smaller_parts(self, max_size: int) -> None:
        raise NotImplementedError('Udp packets cannot be split')
