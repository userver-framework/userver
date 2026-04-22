import pytest_userver.client

import testsuite.utils.http


# /// [service_client]
async def test_ping(service_client: pytest_userver.client.Client):
    response: testsuite.utils.http.ClientResponse = await service_client.get('/ping')
    assert response.status == 200
    # /// [service_client]
