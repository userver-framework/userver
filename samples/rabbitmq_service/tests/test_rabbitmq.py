async def test_rabbitmq(service_client):
    for i in range(10):
        response = await service_client.post('v1/publish', data={
            'message': str(i)
        })
        assert response.status_code == 200
