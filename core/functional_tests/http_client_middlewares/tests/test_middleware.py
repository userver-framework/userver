import asyncio

import pytest


async def test_middleware(service_client, mockserver):
    @mockserver.json_handler('/echo')
    async def _handler(request):
        assert request.headers['additional-header'] == 'header-value'
        return mockserver.make_response()

    response = await service_client.get(
        '/echo',
    )
    assert _handler.times_called == 1
    assert response.status_code == 200


@pytest.mark.uservice_oneshot
async def test_await_detached_request(service_client, mockserver):
    @mockserver.json_handler('/echo')
    async def _handler(request):
        assert request.headers['additional-header'] == 'header-value'
        await asyncio.sleep(3)
        return mockserver.make_response()

    response = await service_client.get(
        '/echo-detach',
    )
    assert response.status_code == 200
    # without awaiting detached requests in HttpClient there would be segfault at teardown
