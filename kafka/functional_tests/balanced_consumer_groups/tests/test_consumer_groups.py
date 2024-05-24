import logging


import pytest


from common import CONSUMERS
from common import get_consumed_messages
from common import parse_message_keys
from common import start_consumers
from common import stop_consumers
from common import TOPIC


async def test_partitions_share(service_client, testpoint, kafka_producer):
    @testpoint('tp_kafka-consumer-first')
    def first_consumer_received(_data):
        pass

    @testpoint('tp_kafka-consumer-first_subscribed')
    def first_consumer_subscribed(_data):
        logging.info('First consumer subscribed')

    @testpoint('tp_kafka-consumer-second')
    def second_consumer_received(_data):
        pass

    @testpoint('tp_kafka-consumer-second_subscribed')
    def second_consumer_subscribed(_data):
        logging.info('Second consumer subscribed')

    await service_client.enable_testpoints()

    await start_consumers(service_client)

    await first_consumer_subscribed.wait_call()
    await second_consumer_subscribed.wait_call()

    await kafka_producer.produce(TOPIC, 'key-1', 'message-1', 0)
    await kafka_producer.produce(TOPIC, 'key-2', 'message-2', 1)

    await first_consumer_received.wait_call()
    await second_consumer_received.wait_call()

    first_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[0],
    )
    second_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[1],
    )

    assert (
        len(first_consumer_messages) == 1
        and len(second_consumer_messages) == 1
    )

    messages: set[str] = set(
        parse_message_keys(first_consumer_messages)
        + parse_message_keys(second_consumer_messages),
    )
    assert messages == set(['key-1', 'key-2'])

    await stop_consumers(service_client)


async def test_rebalance_after_one_consumer_stopped(
        service_client, testpoint, kafka_producer,
):
    @testpoint('tp_kafka-consumer-first')
    def first_consumer_received(_data):
        pass

    @testpoint('tp_kafka-consumer-first_subscribed')
    def first_consumer_subscribed(_data):
        logging.info('First consumer subscribed')

    @testpoint('tp_kafka-consumer-first_revoked')
    def first_consumer_revoked(_data):
        logging.info('First consumer revoked')

    @testpoint('tp_kafka-consumer-second')
    def second_consumer_received(_data):
        pass

    @testpoint('tp_kafka-consumer-second_subscribed')
    def second_consumer_subscribed(_data):
        logging.info('Second consumer subscribed')

    await service_client.enable_testpoints()

    await start_consumers(service_client)

    await first_consumer_subscribed.wait_call()
    await second_consumer_subscribed.wait_call()

    await kafka_producer.produce(TOPIC, 'key-1', 'message-1', 0)
    await kafka_producer.produce(TOPIC, 'key-2', 'message-2', 1)

    await first_consumer_received.wait_call()
    await second_consumer_received.wait_call()

    await stop_consumers(service_client, [CONSUMERS[1]])

    first_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[0],
    )
    second_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[1],
    )
    assert (
        len(first_consumer_messages) == 1
        and len(second_consumer_messages) == 1
    )
    second_consumer_partition = second_consumer_messages[0]['partition']
    logging.info(
        f'Second consumer was subscribed to {second_consumer_partition} partition',
    )

    await kafka_producer.produce(
        TOPIC, 'key-3', 'message-3', second_consumer_partition,
    )
    await first_consumer_revoked.wait_call()
    await first_consumer_subscribed.wait_call()
    await first_consumer_subscribed.wait_call()

    await kafka_producer.produce(
        TOPIC, 'key-4', 'message-4', 1 - second_consumer_partition,
    )

    await first_consumer_received.wait_call()
    await first_consumer_received.wait_call()

    first_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[0],
    )
    assert set(['key-3', 'key-4']) == set(
        parse_message_keys(first_consumer_messages),
    )

    await stop_consumers(service_client, [CONSUMERS[0]])


async def test_rebalance_after_second_consumer_came_after_subscription(
        service_client, testpoint, kafka_producer,
):
    @testpoint('tp_kafka-consumer-first')
    def first_consumer_received(_data):
        pass

    @testpoint('tp_kafka-consumer-first_subscribed')
    def first_consumer_subscribed(_data):
        logging.info('First consumer subscribed')

    @testpoint('tp_kafka-consumer-first_revoked')
    def first_consumer_revoked(_data):
        logging.info('First consumer revoked')

    @testpoint('tp_kafka-consumer-second')
    def second_consumer_received(_data):
        pass

    @testpoint('tp_kafka-consumer-second_subscribed')
    def second_consumer_subscribed(_data):
        logging.info('Second consumer subscribed')

    await service_client.enable_testpoints()

    await start_consumers(service_client, [CONSUMERS[0]])

    await first_consumer_subscribed.wait_call()
    await first_consumer_subscribed.wait_call()

    await kafka_producer.produce(TOPIC, 'key-1', 'message-1', 0)
    await kafka_producer.produce(TOPIC, 'key-2', 'message-2', 1)

    await first_consumer_received.wait_call()
    await first_consumer_received.wait_call()

    first_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[0],
    )
    assert len(first_consumer_messages) == 2
    assert set(['key-1', 'key-2']) == set(
        parse_message_keys(first_consumer_messages),
    )

    await start_consumers(service_client, [CONSUMERS[1]])
    await first_consumer_revoked.wait_call()
    await first_consumer_revoked.wait_call()
    await second_consumer_subscribed.wait_call()
    await first_consumer_subscribed.wait_call()

    await kafka_producer.produce(TOPIC, 'key-3', 'message-3', 0)
    await kafka_producer.produce(TOPIC, 'key-4', 'message-4', 1)

    await first_consumer_received.wait_call()
    await second_consumer_received.wait_call()

    first_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[0],
    )
    second_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[1],
    )

    assert (
        len(first_consumer_messages) == 1
        and len(second_consumer_messages) == 1
    )

    messages: set[str] = set(
        parse_message_keys(first_consumer_messages)
        + parse_message_keys(second_consumer_messages),
    )
    assert messages == set(['key-3', 'key-4'])

    await stop_consumers(service_client)


@pytest.mark.parametrize('exchange_order', ['stop_start', 'start_stop'])
async def test_rebalance_full_partitions_exchange(
        service_client, testpoint, kafka_producer, exchange_order,
):
    @testpoint('tp_kafka-consumer-first')
    def first_consumer_received(_data):
        pass

    @testpoint('tp_kafka-consumer-first_subscribed')
    def first_consumer_subscribed(_data):
        logging.info('First consumer subscribed')

    @testpoint('tp_kafka-consumer-first_revoked')
    def first_consumer_revoked(_data):
        logging.info('First consumer revoked')

    @testpoint('tp_kafka-consumer-first_stopped')
    def first_consumer_stopped(_data):
        logging.info('First consumer stopped')

    @testpoint('tp_kafka-consumer-second')
    def second_consumer_received(_data):
        pass

    @testpoint('tp_kafka-consumer-second_subscribed')
    def second_consumer_subscribed(_data):
        logging.info('Second consumer subscribed')

    @testpoint('tp_kafka-consumer-second_revoked')
    def second_consumer_revoked(_data):
        logging.info('Second consumer revoked')

    await service_client.enable_testpoints()

    await start_consumers(service_client, [CONSUMERS[0]])

    await first_consumer_subscribed.wait_call()
    await first_consumer_subscribed.wait_call()

    await kafka_producer.produce(TOPIC, 'key-1', 'message-1', 0)
    await kafka_producer.produce(TOPIC, 'key-2', 'message-2', 1)

    await first_consumer_received.wait_call()
    await first_consumer_received.wait_call()

    first_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[0],
    )
    assert len(first_consumer_messages) == 2
    assert set(['key-1', 'key-2']) == set(
        parse_message_keys(first_consumer_messages),
    )

    if exchange_order == 'stop_start':
        await stop_consumers(service_client, [CONSUMERS[0]])
        await first_consumer_revoked.wait_call()
        await first_consumer_revoked.wait_call()
        await first_consumer_stopped.wait_call()

        await start_consumers(service_client, [CONSUMERS[1]])
        await second_consumer_subscribed.wait_call()
        await second_consumer_subscribed.wait_call()
    elif exchange_order == 'start_stop':
        await start_consumers(service_client, [CONSUMERS[1]])
        await first_consumer_revoked.wait_call()
        await first_consumer_revoked.wait_call()
        await second_consumer_subscribed.wait_call()
        await first_consumer_subscribed.wait_call()

        await stop_consumers(service_client, [CONSUMERS[0]])
        await first_consumer_revoked.wait_call()
        await first_consumer_stopped.wait_call()

        await second_consumer_revoked.wait_call()
        await second_consumer_subscribed.wait_call()
        await second_consumer_subscribed.wait_call()

    await kafka_producer.produce(TOPIC, 'key-3', 'message-3', 0)
    await kafka_producer.produce(TOPIC, 'key-4', 'message-4', 1)

    await second_consumer_received.wait_call()
    await second_consumer_received.wait_call()

    second_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[1],
    )

    assert len(second_consumer_messages) == 2

    assert set(['key-3', 'key-4']) == set(
        parse_message_keys(second_consumer_messages),
    )

    await stop_consumers(service_client, [CONSUMERS[1]])
