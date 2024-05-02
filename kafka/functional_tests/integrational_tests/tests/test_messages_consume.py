import logging


from confluent_kafka import Producer
import pytest


import common


TOPIC1 = 'test-topic-consume-1'
TOPIC2 = 'test-topic-consume-2'


def _callback(err, msg):
    if err is not None:
        logging.error(
            f'Failed to deliver message to topic {msg.topic()}: {str(err)}',
        )
    else:
        logging.info(
            f'Message produced to topic {msg.topic()} with key {msg.key()}',
        )


@pytest.fixture(name='kafka_producer', scope='session')
def kafka_producer():
    class Wrapper:
        def __init__(self):
            self.producer = Producer(common.get_kafka_conf())

        async def produce(self, topic, key, value, callback=_callback):
            self.producer.produce(
                topic, value=value, key=key, on_delivery=callback,
            )
            self.producer.flush()

    return Wrapper()


async def test_consume_one_message_one_topic(
        service_client, testpoint, kafka_producer,
):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()

    await kafka_producer.produce(TOPIC1, 'test-key', 'test-value')

    await received_messages_func.wait_call()

    response = await service_client.post(
        f'{common.CONSUME_BASE_ROUTE}/{TOPIC1}',
    )
    assert response.status_code == 200
    assert response.json() == {
        'messages': [
            {'topic': TOPIC1, 'key': 'test-key', 'payload': 'test-value'},
        ],
    }


async def test_consume_many_messages_many_topics(
        service_client, testpoint, kafka_producer,
):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()

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
        for message in messages[topic]:
            await kafka_producer.produce(
                message['topic'], message['key'], message['payload'],
            )

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
