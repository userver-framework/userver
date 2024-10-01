async def test_kafka_basic(service_client, testpoint):
    received_key = ''

    @testpoint('message_consumed')
    def message_consumed(data):
        nonlocal received_key
        received_key = data['key']

    await service_client.update_server_state()

    TOPIC1 = 'test-topic-1'
    TOPIC2 = 'test-topic-2'
    MESSAGE_COUNT = 10

    for send in range(MESSAGE_COUNT):
        topic = TOPIC1 if send % 2 == 0 else TOPIC2
        send_key = f'test-key-{send}'
        response = await service_client.post(
            '/produce',
            json={
                'topic': topic,
                'key': send_key,
                'payload': f'test-msg-{send}',
            },
        )
        assert response.status_code == 200

        await message_consumed.wait_call()
        assert received_key == send_key
