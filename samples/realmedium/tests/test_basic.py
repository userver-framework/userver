async def test_basic(service_client):
    response = await service_client.get('/api/tags', params={})
    assert response.status == 200
