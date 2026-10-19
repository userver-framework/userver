import contextlib
import logging
import socket

import h2.connection
import h2.events
import httpx
import pytest

import utils

pytest_plugins = ['pytest_userver.plugins.core']

DEFAULT_TIMEOUT = 10.0


@pytest.fixture(name='create_socket')
async def _create_socket(service_port, asyncio_socket):
    @contextlib.asynccontextmanager
    async def create_socket():
        logging.debug('Connecting to localhost:%d', service_port)
        try:
            sock = asyncio_socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            await sock.connect(('localhost', service_port))
            logging.debug('Connected: %r', sock)

            yield sock

        finally:
            sock.close()

    return create_socket


@pytest.fixture(name='create_connection')
async def _create_connection(create_socket):
    @contextlib.asynccontextmanager
    async def create_connection():
        conn = h2.connection.H2Connection()
        conn.initiate_connection()

        async with create_socket() as sock:
            events = []
            while len(events) != 2:
                events += await utils.send_and_receive(sock, conn)
            assert isinstance(events[0], h2.events.RemoteSettingsChanged)
            assert utils.MAX_CONCURRENT_STREAMS == events[0].changed_settings[3].new_value
            assert utils.DEFAULT_FRAME_SIZE == events[0].changed_settings[5].new_value
            assert isinstance(events[1], h2.events.SettingsAcknowledged)

            logging.debug('Connection successfully created')

            yield sock, conn

    return create_connection


class Http2Client:
    def __init__(self, baseurl):
        self.baseurl = baseurl[:-1]  # rm '/'
        self.client = httpx.AsyncClient(http1=False, http2=True)

    async def __aenter__(self):
        return self

    async def __aexit__(self, *excinfo):
        await self.client.aclose()

    async def get(
        self,
        path,
        params={},
        headers={},
        data=None,
        json={},
        timeout=DEFAULT_TIMEOUT,
    ) -> httpx.Response:
        return await self._request(
            'GET',
            path,
            params,
            headers,
            data,
            json,
            timeout,
        )

    async def post(
        self,
        path,
        params={},
        headers={},
        data=None,
        json={},
        timeout=DEFAULT_TIMEOUT,
    ) -> httpx.Response:
        return await self._request(
            'POST',
            path,
            params,
            headers,
            data,
            json,
            timeout,
        )

    async def put(
        self,
        path,
        params={},
        headers={},
        data=None,
        json={},
        timeout=DEFAULT_TIMEOUT,
    ) -> httpx.Response:
        return await self._request(
            'PUT',
            path,
            params,
            headers,
            data,
            json,
            timeout,
        )

    async def delete(
        self,
        path,
        params={},
        headers={},
        data=None,
        json={},
        timeout=DEFAULT_TIMEOUT,
    ) -> httpx.Response:
        return await self._request(
            'DELETE',
            path,
            params,
            headers,
            data,
            json,
            timeout,
        )

    async def head(
        self,
        path,
        params={},
        headers={},
        data=None,
        json={},
        timeout=DEFAULT_TIMEOUT,
    ) -> httpx.Response:
        return await self._request(
            'HEAD',
            path,
            params,
            headers,
            data,
            json,
            timeout,
        )

    async def trace(
        self,
        path,
        params={},
        headers={},
        data=None,
        json={},
        timeout=DEFAULT_TIMEOUT,
    ) -> httpx.Response:
        return await self._request(
            'TRACE',
            path,
            params,
            headers,
            data,
            json,
            timeout,
        )

    async def _request(
        self,
        method,
        path,
        params,
        headers,
        data,
        json,
        timeout,
    ) -> httpx.Response:
        req = self.client.build_request(
            method,
            self.baseurl + path,
            params=params,
            headers=headers,
            data=data,
            json=json,
            timeout=timeout,
        )
        return await self.client.send(req)


@pytest.fixture
async def http2_client(service_baseurl, service_client) -> Http2Client:
    async with Http2Client(service_baseurl) as client:
        yield client
