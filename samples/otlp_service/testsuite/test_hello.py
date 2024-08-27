async def test_ping(service_client):
    response = await service_client.get('/ping')
    assert response.status == 200
