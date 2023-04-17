async def test_cache_update(service_client):
    response = await service_client.get('/cache/state')
    assert response.status == 200
    assert response.text == 'Magic numbers'
