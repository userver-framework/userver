import logging
from typing import Awaitable


CONSUME_BASE_ROUTE = '/consume'
PRODUCE_ROUTE = '/produce'


def _producer_num_to_name(producer_num: int) -> str:
    suffix = ['first', 'second'][producer_num]

    return f'kafka-producer-{suffix}'


def make_producer_request_body(
        producer_num: int, topic: str, key: str, message: str,
) -> dict[str, str]:
    return {
        'producer': _producer_num_to_name(producer_num),
        'topic': topic,
        'key': key,
        'payload': message,
    }


def produce_async(
        service_client, producer_num: int, topic: str, key: str, message: str,
) -> Awaitable:
    return service_client.post(
        PRODUCE_ROUTE,
        json=make_producer_request_body(producer_num, topic, key, message),
    )


async def produce(
        service_client, producer_num: int, topic: str, key: str, message: str,
) -> Awaitable[None]:
    response = await produce_async(
        service_client, producer_num, topic, key, message,
    )

    assert response.status_code == 200

    return


async def produce_batch(
        service_client, requests: list[dict[str, str]],
) -> Awaitable[None]:
    response = await service_client.post(PRODUCE_ROUTE, json=requests)

    assert response.status_code == 200

    return


async def consume(
        service_client, topic: str,
) -> dict[str, list[dict[str, str]]]:
    response = await service_client.post(f'{CONSUME_BASE_ROUTE}/{topic}')

    assert response.status_code == 200

    return response.json()


async def consume_topic_messages(
        service_client, topic: str, messages: list[dict[str, str]],
) -> Awaitable[None]:
    consumed = await consume(service_client, topic)

    logging.info(f'Messages: {messages}')
    consumed_messages = consumed['messages']
    for consumed_message in consumed_messages:
        logging.info(f'consumed: {consumed_message}')
        messages[topic].remove(consumed_message)
        logging.info(
            f'topic messages {len(messages[topic])}: {messages[topic]}',
        )

    return


async def clear_topics(
        service_client, received_messages_func, messages_to_clear_cnt: int,
) -> Awaitable[None]:
    while messages_to_clear_cnt > 0:
        await received_messages_func.wait_call()

        response = await service_client.post(f'{CONSUME_BASE_ROUTE}/')

        assert response.status_code == 200
        cleared_cnt = len(response.json()['messages'])
        logging.info(f'Cleared {cleared_cnt} messages')
        messages_to_clear_cnt -= cleared_cnt

    return
