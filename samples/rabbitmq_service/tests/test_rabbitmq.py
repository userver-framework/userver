async def test_rabbitmq(testpoint, service_client):
    @testpoint('message_consumed')
    def message_consumed(data):
        pass

    num_messages = 10

    for i in range(num_messages):
        response = await service_client.post(
            '/v1/messages/', json={'message': str(i)},
        )
        assert response.status_code == 200

    for i in range(num_messages):
        await message_consumed.wait_call()

    response = await service_client.get('/v1/messages')
    assert response.status_code == 200

    assert response.json()['messages'] == [i for i in range(num_messages)]
