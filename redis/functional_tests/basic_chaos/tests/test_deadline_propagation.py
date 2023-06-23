async def test_expired(service_client):
    response = await service_client.post(
        '/chaos?key=foo&value=bar&sleep_ms=1000',
        headers={'X-YaTaxi-Client-TimeoutMs': '500'},
    )
    assert response.status == 503
    assert response.text == 'timeout'
