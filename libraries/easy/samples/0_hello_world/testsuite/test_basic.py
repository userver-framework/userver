async def test_hello_simple(service_client):
    response = await service_client.get('/hello')
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'Hello world'
