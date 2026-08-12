import asyncio

import pytest

MESSAGES = [
    {'message': 'a'},
    {'message': 'b', 'reply_to': 'reply_from_b'},
    {'message': 'c', 'correlation_id': 'id_from_c'},
    {'message': 'd', 'correlation_id': 'id_from_d', 'reply_to': 'reply_from_d'},
]


async def _start_consumer(service_client):
    response = await service_client.patch('/v1/messages?action=start')
    assert response.status_code == 200


async def _stop_consumer(service_client):
    response = await service_client.patch('/v1/messages?action=stop')
    assert response.status_code == 200


async def _publish_message(service_client, message):
    request_str = f'/v1/messages?message={message["message"]}&reliable=1'
    if 'reply_to' in message:
        request_str += f'&reply_to={message["reply_to"]}'
    if 'correlation_id' in message:
        request_str += f'&correlation_id={message["correlation_id"]}'
    response = await service_client.post(
        request_str,
    )
    assert response.status_code == 200


async def _clear_messages(service_client):
    response = await service_client.delete('/v1/messages')
    assert response.status_code == 200


def _strip_headers(messages):
    return [{key: value for key, value in message.items() if key != 'headers'} for message in messages]


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

    assert _strip_headers(response.json()) == MESSAGES


async def test_rabbitmq_happy(testpoint, service_client, gate):
    await _clear_messages(service_client)

    await _publish_and_consume(testpoint, service_client)


async def test_rabbitmq_headers(testpoint, service_client, gate):
    await _clear_messages(service_client)

    @testpoint('message_consumed')
    def message_consumed(data):
        pass

    expected_headers = {
        'x-bool': True,
        'x-int': -10,
        'x-uint': 10,
        'x-double': 2.5,
        'x-array': [-7, 'array-value', {'enabled': False, 'nullable': None}],
        'x-object': {
            'count': 42,
            'name': 'nested-object',
            'array': [-7, 'array-value', {'enabled': False, 'nullable': None}],
        },
        'x-null': None,
    }

    response = await service_client.post(
        '/v1/messages?message=headers&reliable=1&reply_to=reply&correlation_id=corr-id',
        json={'headers': expected_headers},
    )
    assert response.status_code == 200

    await message_consumed.wait_call()

    response = await service_client.get('/v1/messages')
    assert response.status_code == 200
    messages = response.json()
    assert len(messages) == 1

    consumed = messages[0]
    assert consumed['message'] == 'headers'
    assert consumed['reply_to'] == 'reply'
    assert consumed['correlation_id'] == 'corr-id'
    assert consumed['headers']['x-bool'] is True
    assert consumed['headers']['x-int'] == -10
    assert consumed['headers']['x-uint'] == 10
    assert consumed['headers']['x-double'] == 2.5
    assert consumed['headers']['x-array'] == expected_headers['x-array']
    assert consumed['headers']['x-object'] == expected_headers['x-object']
    assert consumed['headers']['x-null'] is None
    assert consumed['headers']['u-trace-id']
    assert consumed['headers']['u-parent-span-id']


@pytest.mark.skip(reason='std::terminate is called, fix in TAXICOMMON-6995')
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

    assert _strip_headers(response.json()) == MESSAGES


async def test_rabbitmq_heartbeat_reconnects(testpoint, service_client, gate):
    await _clear_messages(service_client)

    @testpoint('message_consumed')
    def message_consumed(data):
        pass

    response = await service_client.post('/v1/messages?message=before-heartbeat')
    assert response.status_code == 200
    await message_consumed.wait_call()

    async with service_client.capture_logs() as capture:

        @capture.subscribe(text="Consumer for queue 'chaos-queue' is broken, trying to restart")
        def consumer_broken(**kwargs):
            pass

        @capture.subscribe(text='Restarted successfully')
        def consumer_restarted(**kwargs):
            pass

        await gate.to_server_drop()
        await asyncio.sleep(4.0)
        await gate.to_server_pass()

        await consumer_broken.wait_call()
        await consumer_restarted.wait_call()

        response = await service_client.post('/v1/messages?message=after-heartbeat')
        assert response.status_code == 200

        await message_consumed.wait_call()

    response = await service_client.get('/v1/messages')
    assert response.status_code == 200
    assert any(message['message'] == 'after-heartbeat' for message in response.json())
