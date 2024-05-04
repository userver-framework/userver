from common import generate_messages_to_consume
from utils import consume
from utils import consume_topic_messages
from utils import make_producer_request_body
from utils import produce
from utils import produce_batch


TOPIC1 = 'test-topic-consume-produced-1'
TOPIC2 = 'test-topic-consume-produced-2'


async def test_one_producer_sync_one_consumer_one_topic(
        service_client, testpoint,
):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()

    await produce(service_client, 0, TOPIC1, 'test-key', 'test-message')

    await received_messages_func.wait_call()

    consumed = await consume(service_client, TOPIC1)
    assert consumed == {
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

    topics: list[str] = [TOPIC1, TOPIC2]
    messages: dict[str, list[dict[str, str]]] = generate_messages_to_consume(
        topics=topics, cnt=15,
    )

    for topic in topics:
        for i, message in enumerate(messages[topic]):
            await produce(
                service_client,
                i % 2,
                message['topic'],
                message['key'],
                message['payload'],
            )

    while sum([len(messages[topic]) for topic in topics]) > 0:
        await received_messages_func.wait_call()

        for topic in topics:
            await consume_topic_messages(service_client, topic, messages)


async def test_many_producers_async_one_consumer_many_topic(
        service_client, testpoint,
):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()

    topics: list[str] = [TOPIC1, TOPIC2]
    messages: dict[str, list[dict[str, str]]] = generate_messages_to_consume(
        topics=topics, cnt=15,
    )

    requests: list[dict[str, str]] = []
    for topic in topics:
        for i, message in enumerate(messages[topic]):
            requests.append(
                make_producer_request_body(
                    i % 2,
                    message['topic'],
                    message['key'],
                    message['payload'],
                ),
            )

    await produce_batch(service_client, requests)

    while sum([len(messages[topic]) for topic in topics]) > 0:
        await received_messages_func.wait_call()

        for topic in topics:
            await consume_topic_messages(service_client, topic, messages)
