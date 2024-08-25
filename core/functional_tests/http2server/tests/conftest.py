import httpx
import pytest

pytest_plugins = ['pytest_userver.plugins.core']


class Http2Client:
    def __init__(self, baseurl):
        self.baseurl = baseurl[:-1]  # rm '/'
        self.client = httpx.AsyncClient(http1=False, http2=True)

    async def __aenter__(self):
        return self

    async def __aexit__(self, *excinfo):
        await self.client.aclose()

    async def get(
            self, path, params={}, headers={}, data=None, json={}, timeout=5.0,
    ) -> httpx.Response:
        return await self._request(
            'GET', path, params, headers, data, json, timeout,
        )

    async def post(
            self, path, params={}, headers={}, data=None, json={}, timeout=5.0,
    ) -> httpx.Response:
        return await self._request(
            'POST', path, params, headers, data, json, timeout,
        )

    async def put(
            self, path, params={}, headers={}, data=None, json={}, timeout=5.0,
    ) -> httpx.Response:
        return await self._request(
            'PUT', path, params, headers, data, json, timeout,
        )

    async def delete(
            self, path, params={}, headers={}, data=None, json={}, timeout=5.0,
    ) -> httpx.Response:
        return await self._request(
            'DELETE', path, params, headers, data, json, timeout,
        )

    async def _request(
            self, method, path, params, headers, data, json, timeout,
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
