import pytest


@pytest.mark.config(POSTGRES_DEADLINE_PROPAGATION_VERSION=1)
async def test_expired(service_client):
    response = await service_client.post(
        '/chaos/postgres?sleep_ms=1000&type=select',
        headers={'X-YaTaxi-Client-TimeoutMs': '500'},
    )
    assert response.status == 503
    assert response.text == 'timeout'
