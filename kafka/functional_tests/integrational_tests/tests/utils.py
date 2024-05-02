import common


def _producer_num_to_name(producer_num: int) -> str:
    suffix = ['first', 'second'][producer_num]

    return f'kafka-producer-{suffix}'


def _make_producer_request_body(
        producer_num: int, topic: str, key: str, message: str,
) -> dict[str, str]:
    return {
        'producer': _producer_num_to_name(producer_num),
        'topic': topic,
        'key': key,
        'payload': message,
    }


async def prepare_producer(service_client):
    """
        Kafka producer has a cold start issue:
        first request may take a lot of time
        because of metadata fetching
    """

    health = 0
    for i in range(2):
        for _ in range(5):
            response = await service_client.post(
                common.PRODUCE_ROUTE,
                json=_make_producer_request_body(
                    i, 'prepare-topic', 'test', 'test',
                ),
            )

            if response.status == 200:
                health += 1
                break

    return health == 2
