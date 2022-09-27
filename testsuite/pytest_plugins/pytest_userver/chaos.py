import asyncio
import dataclasses
import errno
import fcntl
import logging
import os
import random
import socket
import time
import typing

# https://docs.python.org/3/library/socket.html#socket.socket.recv
RECV_BUFF_SIZE_HINT = 4096

MIN_DELAY = 0.01


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


async def _intercept_ok(
        loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    data = await loop.sock_recv(socket_from, RECV_BUFF_SIZE_HINT)
    await loop.sock_sendall(socket_to, data)


async def _intercept_noop(
        loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    await asyncio.sleep(MIN_DELAY)


async def _intercept_delay(
        delay: float, loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    data = await loop.sock_recv(socket_from, RECV_BUFF_SIZE_HINT)
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
    data = await loop.sock_recv(socket_from, RECV_BUFF_SIZE_HINT)
    for i in range(len(data)):  # pylint: disable=consider-using-enumerate
        data[i] = ~data[i]
    await loop.sock_sendall(socket_to, data)


class _InterceptBpsLimit:
    def __init__(self, bytes_per_second: float):
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

        bytes_to_recv = min(int(self._bytes_left), RECV_BUFF_SIZE_HINT)
        if bytes_to_recv > 0:
            data = await loop.sock_recv(socket_from, bytes_to_recv)
            self._bytes_left -= len(data)

            await loop.sock_sendall(socket_to, data)
        else:
            logger.info('Socket hits the bytes per second limit')
            await asyncio.sleep(max(1.0 / self._bytes_per_second, MIN_DELAY))


class _InterceptTimeLimit:
    def __init__(self, timeout: float, jitter: float):
        self._sockets: typing.Dict[Socket, float] = {}
        self._timeout = timeout
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


async def _intercept_smaller_parts(
        parts_count: int, loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    data = await loop.sock_recv(socket_from, RECV_BUFF_SIZE_HINT)

    length = len(data)
    if length < parts_count:
        await loop.sock_sendall(socket_to, data)
        return

    part_size = (length + parts_count - 1) // parts_count
    for i in range(0, length, part_size):
        await loop.sock_sendall(socket_to, data[i : i + part_size])


class _InterceptBytesLimit:
    def __init__(self, bytes_limit: int, gate: 'TcpGate'):
        self._bytes_limit = bytes_limit
        self._bytes_remain = self._bytes_limit
        self._gate = gate

    async def __call__(
            self, loop: EvLoop, socket_from: Socket, socket_to: Socket,
    ) -> None:
        data = await loop.sock_recv(socket_from, RECV_BUFF_SIZE_HINT)
        self._bytes_remain -= len(data)

        if self._bytes_remain <= 0:
            self._bytes_remain = self._bytes_limit
            await self._gate.sockets_close()
            raise GateInterceptException('Data transmission limit reached')

        await loop.sock_sendall(socket_to, data)


@dataclasses.dataclass(frozen=True)
class GateRoute:
    name: str
    host_left: str
    port_left: int
    host_right: str
    port_right: int


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
            left: socket.socket,
            right: socket.socket,
            to_right_intercept: Interceptor,
            to_left_intercept: Interceptor,
    ) -> None:
        self._route = route
        self._loop = loop

        self._left = left
        self._right = right

        self._to_right_intercept: Interceptor = to_right_intercept
        self._to_left_intercept: Interceptor = to_left_intercept

        self._tasks: typing.Optional[asyncio.Task] = asyncio.create_task(
            self._do_stream(),
        )

    async def _do_stream(self) -> None:
        await asyncio.wait(
            [
                asyncio.create_task(self._do_pipe_channels(to_right=True)),
                asyncio.create_task(self._do_pipe_channels(to_right=False)),
            ],
            return_when=asyncio.ALL_COMPLETED,
        )

    async def _do_pipe_channels(self, *, to_right: bool) -> None:
        if to_right:
            socket_from = self._left
            socket_to = self._right
        else:
            socket_from = self._right
            socket_to = self._left

        try:
            while True:
                # Applies new inreceptors faster.
                #
                # To avoid long awaiting on sock_recv in an outdated
                # interceptor we wait for data before grabbing and applying
                # the interceptor.
                try:
                    socket_from.recv(1, socket.MSG_PEEK)
                except socket.error as e:
                    err = e.args[0]
                    if err in {errno.EAGAIN, errno.EWOULDBLOCK}:
                        await asyncio.sleep(MIN_DELAY)
                        continue
                    else:
                        raise e

                if to_right:
                    interceptor = self._to_right_intercept
                else:
                    interceptor = self._to_left_intercept

                await interceptor(self._loop, socket_from, socket_to)
        except Exception as exc:  # pylint: disable=broad-except
            logger.info('Exception in "%s": %s', self._route.name, exc)
        finally:
            self._close_sockets()

    def set_to_right_interceptor(self, interceptor: Interceptor) -> None:
        self._to_right_intercept = interceptor

    def set_to_left_interceptor(self, interceptor: Interceptor) -> None:
        self._to_left_intercept = interceptor

    def _close_sockets(self) -> None:
        try:
            self._left.close()
        except Exception as exc:  # pylint: disable=broad-except
            logger.warning(
                'Exception in "%s" on closing left: %s', self._route.name, exc,
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


class TcpGate:
    """
    Accepts incomming connections on 'left' (host_left, port_left), on each new
    connection on 'left' connects to 'right' (host_right, port_right).

    Asynchronously concurrently passes data from 'left' to 'right' and from
    'right' to 'left', allowing intercepting the data, injecting delays and
    dropping connections.
    """

    def __init__(self, route: GateRoute, loop) -> None:
        self._route = route
        self._loop = loop

        self._to_right_intercept: Interceptor = _intercept_ok
        self._to_left_intercept: Interceptor = _intercept_ok

        self._accept_sock: typing.Optional[socket.socket] = None
        self._accept_task: typing.Optional[asyncio.Task[None]] = None

        self._connected_event = asyncio.Event()

        self._sockets: typing.Set[_SocketsPaired] = set()

    async def __aenter__(self) -> 'TcpGate':
        self.start()
        return self

    async def __aexit__(self, exc_type, exc_value, traceback) -> None:
        await self.stop()

    def start(self) -> None:
        """ Open the socket and start accepting connections on `left` """
        if self._accept_sock:
            return

        self._accept_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._accept_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._accept_sock.bind((self._route.host_left, self._route.port_left))
        self._accept_sock.listen()

        self.start_accepting()

    def start_accepting(self) -> None:
        """ Start accepting incommong connections on `left` """
        assert self._accept_sock
        if self._accept_task:
            return

        self._accept_task = asyncio.create_task(self._do_accept())

    async def stop_accepting(self) -> None:
        """ Stop accepting incommong connections on `left` """
        await _cancel_and_join(self._accept_task)

    async def stop(self) -> None:
        """
        Stop accepting incommong connections on `left`, close all the sockets
        and connections
        """
        if not self._accept_sock:
            return

        self.to_right_pass()
        self.to_left_pass()

        self._accept_sock.close()

        await _cancel_and_join(self._accept_task)
        await self.sockets_close()
        self._accept_sock = None

    def set_to_right_interceptor(self, interceptor: Interceptor) -> None:
        self._to_right_intercept = interceptor
        for x in self._sockets:
            x.set_to_right_interceptor(self._to_right_intercept)

    def set_to_left_interceptor(self, interceptor: Interceptor) -> None:
        self._to_left_intercept = interceptor
        for x in self._sockets:
            x.set_to_left_interceptor(self._to_left_intercept)

    def to_right_pass(self) -> None:
        """ Pass data as is """
        self.set_to_right_interceptor(_intercept_ok)

    def to_left_pass(self) -> None:
        """ Pass data as is """
        self.set_to_left_interceptor(_intercept_ok)

    def to_right_noop(self) -> None:
        """ Do not read data, causing left socket buffer overflows """
        self.set_to_right_interceptor(_intercept_noop)

    def to_left_noop(self) -> None:
        """ Do not read data, causing right socket buffer overflows """
        self.set_to_left_interceptor(_intercept_noop)

    def to_right_delay(self, delay: float) -> None:
        """ Delay data transmission """

        async def _intercept_delay_bound(
                loop: EvLoop, socket_from: Socket, socket_to: Socket,
        ) -> None:
            await _intercept_delay(delay, loop, socket_from, socket_to)

        self.set_to_right_interceptor(_intercept_delay_bound)

    def to_left_delay(self, delay: float) -> None:
        """ Delay data transmission """

        async def _intercept_delay_bound(
                loop: EvLoop, socket_from: Socket, socket_to: Socket,
        ) -> None:
            await _intercept_delay(delay, loop, socket_from, socket_to)

        self.set_to_left_interceptor(_intercept_delay_bound)

    def to_right_close_on_data(self) -> None:
        """ Close on first bytes of data on left """
        self.set_to_right_interceptor(_intercept_close_on_data)

    def to_left_close_on_data(self) -> None:
        """ Close on first bytes of data on right """
        self.set_to_left_interceptor(_intercept_close_on_data)

    def to_right_corrupt_data(self) -> None:
        """ Corrupt data recveid from left """
        self.set_to_right_interceptor(_intercept_corrupt)

    def to_left_corrupt_data(self) -> None:
        """ Corrupt data recveid from right """
        self.set_to_left_interceptor(_intercept_corrupt)

    def to_right_limit_bps(self, bytes_per_second: float) -> None:
        """ Limit bytes per second transmission by network from left """
        self.set_to_right_interceptor(_InterceptBpsLimit(bytes_per_second))

    def to_left_limit_bps(self, bytes_per_second: float) -> None:
        """ Limit bytes per second transmission by network from right """
        self.set_to_left_interceptor(_InterceptBpsLimit(bytes_per_second))

    def to_right_limit_time(self, timeout: float, jitter: float) -> None:
        """ Limit conection lifetime on receive of first bytes on the left """
        self.set_to_right_interceptor(_InterceptTimeLimit(timeout, jitter))

    def to_left_limit_time(self, timeout: float, jitter: float) -> None:
        """ Limit conection lifetime on receive of first bytes on the right """
        self.set_to_left_interceptor(_InterceptTimeLimit(timeout, jitter))

    def to_right_smaller_parts(self, parts_count: int) -> None:
        """ Pass data in smaller parts """

        async def _intercept_smaller_parts_bound(
                loop: EvLoop, socket_from: Socket, socket_to: Socket,
        ) -> None:
            await _intercept_smaller_parts(
                parts_count, loop, socket_from, socket_to,
            )

        self.set_to_right_interceptor(_intercept_smaller_parts_bound)

    def to_left_smaller_parts(self, parts_count: int) -> None:
        """ Pass data in smaller parts """

        async def _intercept_smaller_parts_bound(
                loop: EvLoop, socket_from: Socket, socket_to: Socket,
        ) -> None:
            await _intercept_smaller_parts(
                parts_count, loop, socket_from, socket_to,
            )

        self.set_to_left_interceptor(_intercept_smaller_parts_bound)

    def to_right_limit_bytes(self, bytes_limit: int) -> None:
        """ Drop all connections each `bytes_limit` of data sent by network """
        self.set_to_right_interceptor(_InterceptBytesLimit(bytes_limit, self))

    def to_left_limit_bytes(self, bytes_limit: int) -> None:
        """ Drop all connections each `bytes_limit` of data sent by network """
        self.set_to_left_interceptor(_InterceptBytesLimit(bytes_limit, self))

    def connections_count(self) -> int:
        """ Returns amount of connections going through the gate """
        return len(self._sockets)

    async def sockets_close(
            self, *, count: typing.Optional[int] = None,
    ) -> None:
        """ Close all the connection going through the gate """
        for x in list(self._sockets)[0:count]:
            await x.shutdown()
        self._collect_garbage()

    async def wait_for_connectons(self, *, count=1, timeout=0.0) -> None:
        """
        Wait for at least `count` connections going through the gate.
        Raises a Timeout exception if failed to get the required amount of
        connections in time.
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

    def _collect_garbage(self) -> None:
        self._sockets = {x for x in self._sockets if x.is_active()}

    async def _do_accept(self) -> None:
        while self._accept_sock:
            left, _ = await self._loop.sock_accept(self._accept_sock)
            left.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            fcntl.fcntl(left, fcntl.F_SETFL, os.O_NONBLOCK)

            right = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            dest = (self._route.host_right, self._route.port_right)
            try:
                await self._loop.sock_connect(right, dest)
                right.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                fcntl.fcntl(right, fcntl.F_SETFL, os.O_NONBLOCK)
                self._sockets.add(
                    _SocketsPaired(
                        self._route,
                        self._loop,
                        left,
                        right,
                        self._to_right_intercept,
                        self._to_left_intercept,
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
                left.close()
                right.close()

            self._collect_garbage()
