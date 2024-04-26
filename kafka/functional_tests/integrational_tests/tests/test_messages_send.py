import asyncio
import logging

import pytest


TOPIC = 'test-topic'

CONSUME_ROUTE = '/consume'
PRODUCE_ROUTE = '/produce'


def _producer_num_to_name(producer_num: int) -> str:
    suffix = ['first', 'second'][producer_num]

    return f'kafka-producer-{suffix}'


def _make_request_body(
        producer_num: int, key: str, message: str,
) -> dict[str, str]:
    return {
        'producer': _producer_num_to_name(producer_num),
        'topic': TOPIC,
        'key': key,
        'payload': message,
    }


@pytest.mark.skip(reason='Kafka error: Broker: Invalid replication factor')
async def test_one_producer_send_sync(service_client):
    response = await service_client.post(
        PRODUCE_ROUTE, json=_make_request_body(0, 'test-key', 'test-message'),
    )
    assert response.status_code == 200

    await asyncio.sleep(2.0)

    response = await service_client.post(CONSUME_ROUTE)

    assert response.status_code == 200
    logging.info(response.json())
    assert response.json() == {
        'messages': [
            {'topic': TOPIC, 'key': 'test-key', 'payload': 'test-message'},
        ],
    }
