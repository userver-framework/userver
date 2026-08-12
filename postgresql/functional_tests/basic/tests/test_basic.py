async def test_insert_then_select(service_client):
    response = await service_client.post('/v1/key-value?key=foo&value=bar')
    assert response.status == 201
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'bar'

    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'bar'


async def test_select_missing(service_client):
    response = await service_client.get('/v1/key-value?key=missing')
    assert response.status == 404
