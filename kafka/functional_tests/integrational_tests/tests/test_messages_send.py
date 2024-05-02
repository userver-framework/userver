import logging
from typing import Awaitable


import common
from utils import _make_producer_request_body
from utils import prepare_producer


TOPIC = 'test-topic-send'


async def _clear_topics(
        service_client, received_messages_func, messages_to_clear_cnt: int,
):
    while messages_to_clear_cnt > 0:
        await received_messages_func.wait_call()

        response = await service_client.post(f'{common.CONSUME_BASE_ROUTE}/')

        assert response.status_code == 200
        cleared_cnt = len(response.json()['messages'])
        logging.info(f'Cleared {cleared_cnt} messages')
        messages_to_clear_cnt -= cleared_cnt


async def test_one_producer_send_sync(service_client, testpoint):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()
    assert await prepare_producer(service_client), 'Failed to prepare producer'

    response = await service_client.post(
        common.PRODUCE_ROUTE,
        json=_make_producer_request_body(0, TOPIC, 'test-key', 'test-message'),
    )

    assert response.status_code == 200

    await _clear_topics(service_client, received_messages_func, 1)


async def test_many_producers_send_sync(service_client, testpoint):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()
    assert await prepare_producer(service_client), 'Failed to prepare producer'

    responses: list[Awaitable[any]] = []
    for i in range(8):
        response = service_client.post(
            common.PRODUCE_ROUTE,
            json=_make_producer_request_body(
                i % 2, TOPIC, f'test-key-{i}', f'test-message-{i}',
            ),
        )
        responses.append(response)

    assert all([(await resp).status_code == 200 for resp in responses])

    await _clear_topics(service_client, received_messages_func, 8)


async def test_many_producers_send_async(service_client, testpoint):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()
    assert await prepare_producer(service_client), 'Failed to prepare producer'

    requests: list[dict[str, str]] = []
    for i in range(8):
        requests.append(
            _make_producer_request_body(
                i % 2, TOPIC, f'test-key-{i}', f'test-message-{i}',
            ),
        )

    response = await service_client.post(common.PRODUCE_ROUTE, json=requests)

    assert response.status_code == 200

    await _clear_topics(service_client, received_messages_func, 8)
