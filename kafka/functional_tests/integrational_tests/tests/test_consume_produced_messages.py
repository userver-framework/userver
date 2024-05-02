import logging


import common
from utils import _make_producer_request_body
from utils import prepare_producer


TOPIC1 = 'test-topic-consume-produced-1'
TOPIC2 = 'test-topic-consume-produced-2'


async def test_one_producer_sync_one_consumer_one_topic(
        service_client, testpoint,
):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()
    assert await prepare_producer(service_client), 'Failed to prepare producer'

    response = await service_client.post(
        common.PRODUCE_ROUTE,
        json=_make_producer_request_body(
            0, TOPIC1, 'test-key', 'test-message',
        ),
    )

    assert response.status_code == 200

    await received_messages_func.wait_call()

    response = await service_client.post(
        f'{common.CONSUME_BASE_ROUTE}/{TOPIC1}',
    )
    assert response.status_code == 200
    assert response.json() == {
        'messages': [
            {'topic': TOPIC1, 'key': 'test-key', 'payload': 'test-message'},
        ],
    }


async def test_many_producers_sync_one_consumer_many_topic(
        service_client, testpoint,
):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()
    assert await prepare_producer(service_client), 'Failed to prepare producer'

    topics = [TOPIC1, TOPIC2]
    messages = {}
    for topic in topics:
        messages[topic] = [
            {
                'topic': topic,
                'key': f'test-key-{i}',
                'payload': f'test-value-{i}',
            }
            for i in range(15)
        ]

    for topic in topics:
        for i, message in enumerate(messages[topic]):
            response = await service_client.post(
                common.PRODUCE_ROUTE,
                json=_make_producer_request_body(
                    i % 2,
                    message['topic'],
                    message['key'],
                    message['payload'],
                ),
            )
            assert response.status_code == 200

    async def consume_topic_messages(topic):
        response = await service_client.post(
            f'{common.CONSUME_BASE_ROUTE}/{topic}',
        )
        assert response.status_code == 200

        consumed_messages = response.json()['messages']
        for consumed_message in consumed_messages:
            logging.info(f'consumed: {consumed_message}')
            logging.info(
                f'topic messages {len(messages[topic])}: {messages[topic]}',
            )
            messages[topic].remove(consumed_message)

    while sum([len(messages[topic]) for topic in topics]) > 0:
        await received_messages_func.wait_call()

        for topic in topics:
            await consume_topic_messages(topic)


async def test_many_producers_async_one_consumer_many_topic(
        service_client, testpoint,
):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()
    assert await prepare_producer(service_client), 'Failed to prepare producer'

    topics = [TOPIC1, TOPIC2]
    messages = {}
    for topic in topics:
        messages[topic] = [
            {
                'topic': topic,
                'key': f'test-key-{i}',
                'payload': f'test-value-{i}',
            }
            for i in range(15)
        ]

    requests: list[dict[str, str]] = []
    for topic in topics:
        for i, message in enumerate(messages[topic]):
            requests.append(
                _make_producer_request_body(
                    i % 2,
                    message['topic'],
                    message['key'],
                    message['payload'],
                ),
            )

    response = await service_client.post(common.PRODUCE_ROUTE, json=requests)
    assert response.status_code == 200

    async def consume_topic_messages(topic):
        response = await service_client.post(
            f'{common.CONSUME_BASE_ROUTE}/{topic}',
        )
        assert response.status_code == 200

        consumed_messages = response.json()['messages']
        for consumed_message in consumed_messages:
            logging.info(f'consumed: {consumed_message}')
            messages[topic].remove(consumed_message)
            logging.info(
                f'topic messages {len(messages[topic])}: {messages[topic]}',
            )

    while sum([len(messages[topic]) for topic in topics]) > 0:
        await received_messages_func.wait_call()

        for topic in topics:
            await consume_topic_messages(topic)
