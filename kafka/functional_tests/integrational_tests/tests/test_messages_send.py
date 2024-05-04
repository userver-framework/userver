from typing import Awaitable


from utils import clear_topics
from utils import make_producer_request_body
from utils import produce
from utils import produce_async
from utils import produce_batch


TOPIC = 'test-topic-send'


async def test_one_producer_send_sync(service_client, testpoint):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()

    await produce(service_client, 0, TOPIC, 'test-key', 'test-message')

    await clear_topics(
        service_client, received_messages_func, messages_to_clear_cnt=1,
    )


async def test_many_producers_send_sync(service_client, testpoint):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()

    responses: list[Awaitable[any]] = []
    for i in range(8):
        responses.append(
            produce_async(
                service_client,
                i % 2,
                TOPIC,
                f'test-key-{i}',
                f'test-message-{i}',
            ),
        )

    assert all([(await resp).status_code == 200 for resp in responses])

    await clear_topics(
        service_client, received_messages_func, messages_to_clear_cnt=8,
    )


async def test_many_producers_send_async(service_client, testpoint):
    @testpoint('tp_kafka-consumer')
    def received_messages_func(_data):
        pass

    await service_client.enable_testpoints()

    requests: list[dict[str, str]] = []
    for i in range(8):
        requests.append(
            make_producer_request_body(
                i % 2, TOPIC, f'test-key-{i}', f'test-message-{i}',
            ),
        )

    await produce_batch(service_client, requests)

    await clear_topics(
        service_client, received_messages_func, messages_to_clear_cnt=8,
    )
