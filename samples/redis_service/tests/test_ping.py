async def test_ping(test_service_client):
    response = await test_service_client.get('/ping')
    assert response.status == 200
