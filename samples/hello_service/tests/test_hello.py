async def test_ping(test_service_client):
    response = await test_service_client.get('/hello')
    assert response.status == 200
    assert response.content == b'Hello world!\n'
