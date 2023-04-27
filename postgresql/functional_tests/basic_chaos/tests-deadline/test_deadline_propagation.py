import pytest


@pytest.mark.config(POSTGRES_DEADLINE_PROPAGATION_ENABLED=True)
async def test_expired(service_client):
    response = await service_client.post(
        '/chaos/postgres?sleep_ms=100&type=select',
        headers={'X-YaTaxi-Client-TimeoutMs': '1'},
    )
    assert response.status == 503
    assert response.text == 'timeout'
