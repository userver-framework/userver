# /// [Functional test]
async def test_postgres(service_client):
    response = await service_client.delete('/v1/key-value?key=hello')
    assert response.status == 200

    response = await service_client.post('/v1/key-value?key=hello&value=world')
    assert response.status == 201
    assert response.content == b'world'

    response = await service_client.get('/v1/key-value?key=hello')
    assert response.status == 200
    assert response.content == b'world'

    response = await service_client.delete('/v1/key-value?key=hello')
    assert response.status == 200

    response = await service_client.post('/v1/key-value?key=hello&value=there')
    assert response.status == 201
    assert response.content == b'there'
    # /// [Functional test]

    response = await service_client.get('/v1/key-value?key=hello')
    assert response.status == 200
    assert response.content == b'there'

    response = await service_client.post('/v1/key-value?key=hello&value=again')
    assert response.status == 409
    assert response.content == b'there'

    response = await service_client.get('/v1/key-value?key=missing')
    assert response.status == 404
