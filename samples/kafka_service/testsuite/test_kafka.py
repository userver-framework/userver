# /// [Kafka service sample - kafka functional test example]
async def test_kafka_basic(service_client, testpoint):
    received_key = ''

    # register testpoint, which consumer calls
    # after each message is consumed
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
        # send message and waits its delivery
        response = await service_client.post(
            '/produce',
            json={
                'topic': topic,
                'key': send_key,
                'payload': f'test-msg-{send}',
            },
        )
        assert response.status_code == 200

        # wait until consume read the message and call the testpoint
        await message_consumed.wait_call()
        # check message key
        assert received_key == send_key


# /// [Kafka service sample - kafka functional test example]
