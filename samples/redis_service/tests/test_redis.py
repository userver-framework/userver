async def test_redis(test_service_client):
    response = await test_service_client.delete('/v1/key-value?key=hello')
    assert response.status == 200

    response = await test_service_client.post(
        '/v1/key-value?key=hello&value=world',
    )
    assert response.status == 201
    assert response.content == b'world'

    response = await test_service_client.request(
        'GET', '/v1/key-value?key=hello',
    )
    assert response.status == 200
    assert response.content == b'world'

    response = await test_service_client.request(
        'POST', '/v1/key-value?key=hello&value=there',
    )
    assert response.status == 409  # Conflict

    response = await test_service_client.request(
        'GET', '/v1/key-value?key=hello',
    )
    assert response.status == 200
    assert response.content == b'world'  # Still the same

    response = await test_service_client.request(
        'DELETE', '/v1/key-value?key=hello',
    )
    assert response.status == 200
    assert response.content == b'1'

    response = await test_service_client.request(
        'GET', '/v1/key-value?key=hello',
    )
    assert response.status == 404  # Not Found
