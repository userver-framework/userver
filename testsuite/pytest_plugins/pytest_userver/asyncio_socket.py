# TODO: move to testsuite
import asyncio
import select
import socket
import sys
import typing

DEFAULT_TIMEOUT = 10.0

_ASYNCIO_HAS_SENDTO = sys.version_info >= (3, 11)


class AsyncioSocket:
    def __init__(
        self,
        sock: socket.socket,
        loop: typing.Optional[asyncio.AbstractEventLoop] = None,
        timeout=DEFAULT_TIMEOUT,
    ):
        if loop is None:
            loop = asyncio.get_running_loop()
        self._loop: asyncio.AbstractEventLoop = loop
        self._sock: socket.socket = sock
        self._default_timeout = timeout
        sock.setblocking(False)

    def __repr__(self):
        return f'<AsyncioSocket for {self._sock}>'

    @property
    def type(self):
        return self._sock.type

    def fileno(self):
        return self._sock.fileno()

    async def connect(self, address, *, timeout=None):
        coro = self._loop.sock_connect(self._sock, address)
        return await self._with_timeout(coro, timeout=timeout)

    async def send(self, data, *, timeout=None):
        coro = self._loop.sock_send(self._sock, data)
        return self._with_timeout(coro, timeout=timeout)

    async def sendto(self, *args, timeout=None):
        # asyncio support added in version 3.11
        if not _ASYNCIO_HAS_SENDTO:
            return await self._sendto_legacy(*args, timeout=timeout)
        coro = self._loop.sock_sendto(self._sock, *args)
        try:
            return await self._with_timeout(coro, timeout=timeout)
        except NotImplementedError:
            return await self._sendto_legacy(*args, timeout=timeout)

    async def sendall(self, data, *, timeout=None):
        coro = self._loop.sock_sendall(self._sock, data)
        return await self._with_timeout(coro, timeout=timeout)

    async def recv(self, size, *, timeout=None):
        coro = self._loop.sock_recv(self._sock, size)
        return await self._with_timeout(coro, timeout=timeout)

    async def recvfrom(self, *args, timeout=None):
        # asyncio support added in version 3.11
        if not _ASYNCIO_HAS_SENDTO:
            return await self._recvfrom_legacy(*args, timeout=timeout)
        coro = self._loop.sock_recvfrom(self._sock, *args)
        try:
            return await self._with_timeout(coro, timeout=timeout)
        except NotImplementedError:
            return await self._recvfrom_legacy(*args, timeout=timeout)

    async def accept(self, *, timeout=None):
        coro = self._loop.sock_accept(self._sock)
        conn, address = await self._with_timeout(coro, timeout=timeout)
        return from_socket(conn), address

    def bind(self, address):
        return self._sock.bind(address)

    def listen(self, *args):
        return self._sock.listen(*args)

    def getsockname(self):
        return self._sock.getsockname()

    def setsockopt(self, *args, **kwargs):
        self._sock.setsockopt(*args, **kwargs)

    def close(self):
        self._sock.close()

    def has_data(self) -> bool:
        rlist, _, _ = select.select([self._sock], [], [], 0)
        return bool(rlist)

    def can_write(self) -> bool:
        _, wlist, _ = select.select([], [self._sock], [], 0)
        return bool(wlist)

    async def _with_timeout(self, awaitable, timeout=None):
        # TODO(python3.11): switch to `asyncio.timeout()`
        if timeout is None:
            timeout = self._default_timeout

        return await asyncio.wait_for(awaitable, timeout=timeout)

    async def _sendto_legacy(self, *args, timeout):
        # uvloop and python < 3.11
        async def do_sendto():
            while not self.can_write():
                await asyncio.sleep(0.001)
            return self._sock.sendto(*args)

        return await self._with_timeout(do_sendto(), timeout=timeout)

    async def _recvfrom_legacy(self, *args, timeout):
        # uvloop and python < 3.11
        async def do_recvfrom():
            while not self.has_data():
                await asyncio.sleep(0.001)
            return self._sock.recvfrom(*args)

        return await self._with_timeout(do_recvfrom(), timeout=timeout)


class AsyncioSocketsFactory:
    def __init__(self, loop=None):
        if loop is None:
            loop = asyncio.get_running_loop()
        self._loop = loop

    def socket(self, *args, timeout=DEFAULT_TIMEOUT):
        sock = socket.socket(*args)
        return from_socket(sock, loop=self._loop, timeout=timeout)

    def tcp(self, *, timeout=DEFAULT_TIMEOUT):
        return self.socket(socket.AF_INET, socket.SOCK_STREAM, timeout=timeout)

    def udp(self, *, timeout=DEFAULT_TIMEOUT):
        return self.socket(socket.AF_INET, socket.SOCK_DGRAM, timeout=timeout)


def from_socket(
    sock: typing.Union[socket.socket, AsyncioSocket],
    *,
    loop=None,
    timeout=DEFAULT_TIMEOUT,
) -> AsyncioSocket:
    if isinstance(sock, AsyncioSocket):
        return sock
    return AsyncioSocket(sock, loop=loop, timeout=timeout)


def create_socket(*args, timeout=DEFAULT_TIMEOUT):
    return AsyncioSocketsFactory().socket(*args, timeout=timeout)


def create_tcp_socket(*args, timeout=DEFAULT_TIMEOUT):
    return AsyncioSocketsFactory().tcp(timeout=timeout)


def create_udp_socket(timeout=DEFAULT_TIMEOUT):
    return AsyncioSocketsFactory().udp(timeout=timeout)
