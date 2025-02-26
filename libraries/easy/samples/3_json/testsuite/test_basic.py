async def test_kv(service_client):
    response = await service_client.post(
        '/kv',
        json={'key': 1, 'value': 'one'},
    )
    assert response.status == 200
    assert response.json() is None
    assert 'application/json' in response.headers['Content-Type']

    response = await service_client.get('/kv', json={'key': 1})
    assert response.status == 200
    assert response.json() == {'key': 1, 'value': 'one'}
    assert 'application/json' in response.headers['Content-Type']

    response = await service_client.post(
        '/kv',
        json={'key': 1, 'value': 'again_1'},
    )
    assert response.status == 200

    response = await service_client.get('/kv', json={'key': 1})
    assert response.status == 200
    assert response.json() == {'key': 1, 'value': 'again_1'}
    assert 'application/json' in response.headers['Content-Type']
