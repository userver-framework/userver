import os


CONSUME_BASE_ROUTE = '/consume'
CONSUMERS = ['kafka-consumer-first', 'kafka-consumer-second']
TOPIC = 'test-topic'


def get_kafka_conf() -> dict[str, str]:
    return {'bootstrap.servers': os.getenv('KAFKA_RECIPE_BROKER_LIST')}


async def start_consumers(service_client, consumers: list[str] = CONSUMERS):
    for consumer in consumers:
        response = await service_client.put(f'{CONSUME_BASE_ROUTE}/{consumer}')
        assert response.status_code == 200


async def stop_consumers(service_client, consumers: list[str] = CONSUMERS):
    for consumer in consumers:
        response = await service_client.delete(
            f'{CONSUME_BASE_ROUTE}/{consumer}',
        )
        assert response.status_code == 200


async def get_consumed_messages(
        service_client, consumer: str,
) -> list[dict[str, str]]:
    response = await service_client.post(f'{CONSUME_BASE_ROUTE}/{consumer}')
    assert response.status_code == 200

    return response.json()['messages']


def parse_message_keys(messages: list[dict[str, str]]) -> list[str]:
    return list(map(lambda message: message['key'], messages))


async def make_non_faulty(service_client, consumer):
    response = await service_client.patch(f'{CONSUME_BASE_ROUTE}/{consumer}')
    assert response.status_code == 200
