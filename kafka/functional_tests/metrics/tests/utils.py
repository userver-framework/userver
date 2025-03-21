from typing import Awaitable
from typing import Dict
from typing import List

CONSUME_BASE_ROUTE = '/consume'
PRODUCE_ROUTE = '/produce'


def make_producer_request_body(
    topic: str,
    key: str,
    message: str,
) -> Dict[str, str]:
    return {
        'producer': 'kafka-producer',
        'topic': topic,
        'key': key,
        'payload': message,
    }


def produce_async(
    service_client,
    topic: str,
    key: str,
    message: str,
) -> Awaitable:
    return service_client.post(
        PRODUCE_ROUTE,
        json=make_producer_request_body(topic, key, message),
    )


async def produce(
    service_client,
    topic: str,
    key: str,
    message: str,
) -> Awaitable[None]:
    response = await produce_async(
        service_client,
        topic,
        key,
        message,
    )

    assert response.status_code == 200

    return


async def consume(
    service_client,
    topic: str,
) -> Dict[str, List[Dict[str, str]]]:
    response = await service_client.post(f'{CONSUME_BASE_ROUTE}/{topic}')

    assert response.status_code == 200

    return response.json()
