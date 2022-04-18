async def test_grpc(service_client):
    response = await service_client.post(
        '/hello', data='tests', headers={'Content-type': 'text/plain'},
    )
    assert response.status == 200
    assert response.content == b'Hello, tests!'
