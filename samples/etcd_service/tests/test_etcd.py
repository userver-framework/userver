# /// [Functional test]
async def test_etcd(service_client):
    response = await service_client.get('/v1/key-value')
    assert response.status == 400

    response = await service_client.post('/v1/key-value')
    assert response.status == 400

    response = await service_client.delete('/v1/key-value')
    assert response.status == 400

    response = await service_client.get('/v1/key-value?key=hello')
    assert response.status == 404

    response = await service_client.delete('/v1/key-value?key=hello')
    assert response.status == 200

    response = await service_client.post('/v1/key-value?key=hello&value=world')
    assert response.status == 200

    response = await service_client.request('GET', '/v1/key-value?key=hello')
    assert response.status == 200
    assert response.json()['hello'] == 'world'

    response = await service_client.delete('/v1/key-value?key=hello')
    assert response.status == 200

    response = await service_client.get('/v1/key-value?key=hello')
    assert response.status == 404

    response = await service_client.post('/v1/key-value?key=hello&value=there')
    assert response.status == 200

    response = await service_client.request('GET', '/v1/key-value?key=hello')
    assert response.status == 200
    assert response.json()['hello'] == 'there'