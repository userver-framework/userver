async def test_kv(service_client):
    response = await service_client.post('/kv?key=1&value=one')
    assert response.status == 200

    response = await service_client.get('/kv?key=1')
    assert response.status == 200
    assert response.text == 'one'

    response = await service_client.post('/kv?key=1&value=again_1')
    assert response.status == 200

    response = await service_client.get('/kv?key=1')
    assert response.status == 200
    assert response.text == 'again_1'
