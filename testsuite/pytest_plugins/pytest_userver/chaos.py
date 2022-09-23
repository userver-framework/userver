import asyncio
import dataclasses
import logging
import socket
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


async def intercept_ok(
        loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    """
    Pass data as is
    """
    data = await loop.sock_recv(socket_from, RECV_BUFF_SIZE_HINT)
    await loop.sock_sendall(socket_to, data)


async def intercept_noop(
        loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    """
    Do not read data, causing socket_from buffer overflows
    """
    await asyncio.sleep(MIN_DELAY)


async def intercept_delay(
        delay: float, loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    """
    Delay data transmission
    """
    data = await loop.sock_recv(socket_from, RECV_BUFF_SIZE_HINT)
    await asyncio.sleep(delay)
    await loop.sock_sendall(socket_to, data)


async def intercept_read_only(
        loop: EvLoop, socket_from: Socket, socket_to: Socket,
) -> None:
    """
    Read data without passing it forward
    """
    await loop.sock_recv(socket_from, RECV_BUFF_SIZE_HINT)


@dataclasses.dataclass(frozen=True)
class GateRoute:
    name: str
    host_left: str
    port_left: int
    host_right: str
    port_right: int


async def _cancel_and_join(task) -> None:
    if not task or task.cancelled():
        return

    task.cancel()
    try:
        await task
    except asyncio.CancelledError:
        pass
    except Exception as exc:  # pylint: disable=broad-except
        logger.warning('Exception: %s', exc)


class SocketsPaired:
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

        self._to_right = None
        self._to_left = None

        self._to_right = asyncio.create_task(
            self._do_pipe_channels(to_right=True),
        )
        self._to_left = asyncio.create_task(
            self._do_pipe_channels(to_right=False),
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
                if to_right:
                    interceptor = self._to_right_intercept
                else:
                    interceptor = self._to_left_intercept

                await interceptor(self._loop, socket_from, socket_to)
        except Exception as exc:  # pylint: disable=broad-except
            logger.info('Exception in "%s": %s', self._route.name, exc)
            await self.shutdown()

    def set_to_right_interceptor(self, interceptor: Interceptor) -> None:
        self._to_right_intercept = interceptor

    def set_to_left_interceptor(self, interceptor: Interceptor) -> None:
        self._to_left_intercept = interceptor

    async def shutdown(self) -> None:
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

        await _cancel_and_join(self._to_right)
        await _cancel_and_join(self._to_left)

        self._to_right = None

    def is_active(self) -> bool:
        return bool(self._to_right)


class Gate:
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

        self._to_right_intercept: Interceptor = intercept_ok
        self._to_left_intercept: Interceptor = intercept_ok

        self._accept_sock: typing.Optional[socket.socket] = None
        self._accept_task: typing.Optional[asyncio.Task[None]] = None

        self._connected_event = asyncio.Event()

        self._sockets: typing.Set[SocketsPaired] = set()

    async def __aenter__(self) -> 'Gate':
        self.start()
        return self

    async def __aexit__(self, exc_type, exc_value, traceback) -> None:
        await self.stop()

    def start(self) -> None:
        if self._accept_sock:
            return

        self._accept_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._accept_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._accept_sock.bind((self._route.host_left, self._route.port_left))
        self._accept_sock.listen()

        self._accept_task = asyncio.create_task(self._do_accept())

    async def stop(self) -> None:
        if not self._accept_sock:
            return

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

    def to_right_delay(self, delay: float) -> None:
        async def _intercept_delay(
                loop: EvLoop, socket_from: Socket, socket_to: Socket,
        ) -> None:
            await intercept_delay(delay, loop, socket_from, socket_to)

        self.set_to_right_interceptor(_intercept_delay)

    def to_left_delay(self, delay: float) -> None:
        async def _intercept_delay(
                loop: EvLoop, socket_from: Socket, socket_to: Socket,
        ) -> None:
            await intercept_delay(delay, loop, socket_from, socket_to)

        self.set_to_left_interceptor(_intercept_delay)

    def to_right_pass(self) -> None:
        self.set_to_right_interceptor(intercept_ok)

    def to_left_pass(self) -> None:
        self.set_to_left_interceptor(intercept_ok)

    def to_right_noop(self) -> None:
        self.set_to_right_interceptor(intercept_noop)

    def to_left_noop(self) -> None:
        self.set_to_left_interceptor(intercept_noop)

    def connections_count(self) -> int:
        return len(self._sockets)

    async def sockets_close(self) -> None:
        for x in self._sockets:
            await x.shutdown()
        self._collect_garbage()

    async def wait_for_connectons(self, *, count=1) -> None:
        while self.connections_count() < count:
            await self._connected_event.wait()
            self._connected_event.clear()

    def _collect_garbage(self) -> None:
        self._sockets = {x for x in self._sockets if x.is_active()}

    async def _do_accept(self) -> None:
        while self._accept_sock:
            left, _ = await self._loop.sock_accept(self._accept_sock)
            left.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

            right = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            dest = (self._route.host_right, self._route.port_right)
            try:
                await self._loop.sock_connect(right, dest)
                right.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                self._sockets.add(
                    SocketsPaired(
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
