# TODO: move to testsuite
import asyncio
import select
import socket
import typing


class AsyncioSocket:
    def __init__(
            self,
            sock: socket.socket,
            loop: typing.Optional[asyncio.AbstractEventLoop] = None,
            timeout=10.0,
    ):
        if loop is None:
            loop = asyncio.get_running_loop()
        self._loop: asyncio.AbstractEventLoop = loop
        self._sock: socket.socket = sock
        self._default_timeout = timeout
        sock.setblocking(False)

    def __repr__(self):
        return f'<AsyncioSocket for {self._sock}>'

    async def connect(self, address, *, timeout=None):
        async with self._timeout(timeout):
            return await self._loop.sock_connect(self._sock, address)

    async def send(self, data, *, timeout=None):
        async with self._timeout(timeout):
            return await self._loop.sock_send(self._sock, data)

    async def sendto(self, *args, timeout=None):
        async with self._timeout(timeout):
            return await self._loop.sock_sendto(self._sock, *args)

    async def sendall(self, data, *, timeout=None):
        async with self._timeout(timeout):
            return await self._loop.sock_sendall(self._sock, data)

    async def recv(self, size, *, timeout=None):
        async with self._timeout(timeout):
            return await self._loop.sock_recv(self._sock, size)

    async def recvfrom(self, *args, timeout=None):
        async with self._timeout(timeout):
            return await self._loop.sock_recvfrom(self._sock, *args)

    async def accept(self, *, timeout=None):
        async with self._timeout(timeout):
            conn, address = await self._loop.sock_accept(self._sock)
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

    def _timeout(self, timeout=None):
        if timeout is None:
            timeout = self._default_timeout
        return asyncio.timeout(timeout)


class AsyncioSocketsFactory:
    def __init__(self, loop=None):
        if loop is None:
            loop = asyncio.get_running_loop()
        self._loop = loop

    def socket(self, *args, timeout=10.0):
        sock = socket.socket(*args)
        return from_socket(sock, loop=self._loop, timeout=timeout)

    def tcp(self, *, timeout=10.0):
        return self.socket(socket.AF_INET, socket.SOCK_INET, timeout=timeout)

    def udp(self, *, timeout=10.0):
        return self.socket(socket.AF_INET, socket.SOCK_DGRAM, timeout=timeout)


def from_socket(
        sock: typing.Union[socket.socket, AsyncioSocket], *, loop=None, timeout=10.0,
) -> AsyncioSocket:
    if isinstance(sock, AsyncioSocket):
        return sock
    return AsyncioSocket(sock, loop=loop, timeout=timeout)
