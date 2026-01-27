import asyncio

import pytest

from testsuite import asyncio_socket


@pytest.fixture(name='asyncio_loop')
async def _asyncio_loop():
    return asyncio.get_running_loop()


@pytest.fixture(name='asyncio_socket')
async def _asyncio_socket(asyncio_loop):
    return asyncio_socket.AsyncioSocketsFactory(loop=asyncio_loop)
