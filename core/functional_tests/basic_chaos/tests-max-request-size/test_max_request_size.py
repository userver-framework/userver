async def test_request_size(service_client):
    response = await service_client.get('/ping')
    assert response.status == 200

    response = await service_client.get('/ping', params={'arg': 'x' * 1000})
    assert response.status == 413
    assert response.text == 'too large request'
