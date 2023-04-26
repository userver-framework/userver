import pytest


@pytest.mark.config(REDIS_DEADLINE_PROPAGATION_ENABLED=True)
async def test_expired(service_client):
    response = await service_client.post(
        '/chaos?key=foo&value=bar&sleep_ms=100',
        headers={'X-YaTaxi-Client-TimeoutMs': '1'},
    )
    assert response.status == 503
    assert response.text == 'timeout'
