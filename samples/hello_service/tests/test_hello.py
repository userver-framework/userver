# /// [Functional test]
async def test_ping(service_client):
    response = await service_client.get('/hello')
    assert response.status == 200
    assert response.content == b'Hello world!\n'
    # /// [Functional test]
