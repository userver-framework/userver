from common import generate_messages_to_consume
from utils import consume
from utils import consume_topic_messages


TOPIC1 = 'test-topic-consume-1'
TOPIC2 = 'test-topic-consume-2'


async def test_consume_one_message_one_topic(
        service_client, testpoint, kafka_producer,
):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()

    await kafka_producer.produce(TOPIC1, 'test-key', 'test-value')

    await received_messages_func.wait_call()

    consumed = await consume(service_client, TOPIC1)
    assert consumed == {
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
    messages: dict[str, list[dict[str, str]]] = generate_messages_to_consume(
        topics=topics, cnt=15,
    )

    for topic in topics:
        for message in messages[topic]:
            await kafka_producer.produce(
                message['topic'], message['key'], message['payload'],
            )

    while sum([len(messages[topic]) for topic in topics]) > 0:
        await received_messages_func.wait_call()

        for topic in topics:
            await consume_topic_messages(service_client, topic, messages)
