async def test_hi_to(service_client):
    response = await service_client.get('/hi/flusk')
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'Hello, flusk'


async def test_hi(service_client):
    response = await service_client.get('/hi?name=pal')
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'Hi, pal'
