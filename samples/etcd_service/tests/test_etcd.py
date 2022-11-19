# /// [Functional test]
async def test_etcd(service_client):
    response = await service_client.delete('/v1/key-value?key=hello')
    assert response.status == 200

    response = await service_client.post('/v1/key-value?key=hello&value=world')
    assert response.status == 200

    response = await service_client.request('GET', '/v1/key-value?key=hello')
    assert response.status == 200
    assert response.json()['hello'] == b'world'
    # /// [Functional test]

    response = await service_client.request(
        'POST', '/v1/key-value?key=hello&value=there',
    )
    assert response.status == 409  # Conflict

    response = await service_client.request('GET', '/v1/key-value?key=hello')
    assert response.status == 200
    assert response.json()['hello'] == b'world'  # Still the same

    response = await service_client.request(
        'DELETE', '/v1/key-value?key=hello',
    )
    assert response.status == 200

    response = await service_client.request('GET', '/v1/key-value?key=hello')
    assert response.status == 404  # Not Found
