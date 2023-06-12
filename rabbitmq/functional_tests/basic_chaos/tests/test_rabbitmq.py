import asyncio


MESSAGES = ['a', 'b', 'c']


async def _start_consumer(service_client):
    response = await service_client.patch('/v1/messages?action=start')
    assert response.status_code == 200


async def _stop_consumer(service_client):
    response = await service_client.patch('/v1/messages?action=stop')
    assert response.status_code == 200


async def _publish_message(service_client, message):
    response = await service_client.post(
        f'/v1/messages?message={message}&reliable=1',
    )
    assert response.status_code == 200


async def _clear_messages(service_client):
    response = await service_client.delete('/v1/messages')
    assert response.status_code == 200


async def _publish_and_consume(testpoint, client):
    @testpoint('message_consumed')
    def message_consumed(data):
        pass

    for message in MESSAGES:
        await _publish_message(client, message)

    for _ in MESSAGES:
        await message_consumed.wait_call()

    response = await client.get('/v1/messages')
    assert response.status_code == 200

    assert response.json() == MESSAGES


async def test_rabbitmq_happy(testpoint, service_client, gate):
    await _clear_messages(service_client)

    await _publish_and_consume(testpoint, service_client)


async def test_consumer_reconnects(testpoint, service_client, gate):
    await _clear_messages(service_client)

    @testpoint('message_consumed')
    def message_consumed(data):
        pass

    await _stop_consumer(service_client)
    for message in MESSAGES:
        response = await _publish_message(service_client, message)
    assert message_consumed.times_called == 0

    await gate.sockets_close()

    await _start_consumer(service_client)
    await asyncio.sleep(1)
    assert message_consumed.times_called == 0

    for _ in MESSAGES:
        await message_consumed.wait_call()

    response = await service_client.get('/v1/messages')
    assert response.status_code == 200

    assert response.json() == MESSAGES
