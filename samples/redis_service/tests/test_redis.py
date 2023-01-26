# /// [Functional test]
async def test_redis(service_client):
    response = await service_client.delete('/v1/key-value?key=hello')
    assert response.status == 200

    response = await service_client.post('/v1/key-value?key=hello&value=world')
    assert response.status == 201
    assert response.content == b'world'

    response = await service_client.request('GET', '/v1/key-value?key=hello')
    assert response.status == 200
    assert response.content == b'world'
    # /// [Functional test]

    response = await service_client.request(
        'POST', '/v1/key-value?key=hello&value=there',
    )
    assert response.status == 409  # Conflict

    response = await service_client.request('GET', '/v1/key-value?key=hello')
    assert response.status == 200
    assert response.content == b'world'  # Still the same

    response = await service_client.request(
        'DELETE', '/v1/key-value?key=hello',
    )
    assert response.status == 200
    assert response.content == b'1'

    response = await service_client.request('GET', '/v1/key-value?key=hello')
    assert response.status == 404  # Not Found


async def test_evalsha(service_client):
    script = 'return "42"'

    hash_id = 'initialhash'
    response = await service_client.post(
        f'/v1/script?command=evalsha&key=hello&hash={hash_id}',
    )
    assert response.status == 200
    assert response.content == b'NOSCRIPT'

    response = await service_client.post(
        f'/v1/script?command=scriptload&key=hello&script={script}',
    )
    assert response.status == 200
    hash_id = response.content.decode()

    response = await service_client.post(
        f'/v1/script?command=evalsha&key=hello&hash={hash_id}',
    )
    assert response.status == 200
    assert response.content == b'42'
