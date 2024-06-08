import logging


from common import CONSUMERS
from common import get_consumed_messages
from common import make_non_faulty
from common import parse_message_keys
from common import start_consumers
from common import stop_consumers
from common import TOPIC


MESSAGE_TO_FAIL = 'please-start-failing'


async def test_rebalance_after_failure(
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

    @testpoint('tp_kafka-consumer-second_polled')
    def second_consumer_polled(_data):
        logging.info('Second consumer polled')

    @testpoint('tp_error_kafka-consumer-second')
    def second_consumer_failed(_data):
        logging.warning('Second consumer failed')

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
    second_consumer_partition = second_consumer_messages[0]['partition']
    logging.info(
        f'Second consumer was subscribed to {second_consumer_partition} partition',
    )

    await kafka_producer.produce(
        TOPIC, 'key-3', MESSAGE_TO_FAIL, second_consumer_partition,
    )
    await second_consumer_polled.wait_call()
    await second_consumer_failed.wait_call()

    await stop_consumers(service_client, [CONSUMERS[1]])

    await first_consumer_revoked.wait_call()
    await first_consumer_subscribed.wait_call()
    await first_consumer_subscribed.wait_call()

    await kafka_producer.produce(
        TOPIC, 'key-4', 'message-4', second_consumer_partition,
    )
    await first_consumer_received.wait_call()
    await first_consumer_received.wait_call()

    first_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[0],
    )
    assert len(first_consumer_messages) == 2

    assert set(['key-3', 'key-4']) == set(
        parse_message_keys(first_consumer_messages),
    )

    await stop_consumers(service_client, [CONSUMERS[0]])


async def test_message_reprocessed_after_failure(
        service_client, testpoint, kafka_producer,
):
    @testpoint('tp_kafka-consumer-second')
    def second_consumer_received(_data):
        pass

    @testpoint('tp_kafka-consumer-second_subscribed')
    def second_consumer_subscribed(_data):
        logging.info('Second consumer subscribed')

    @testpoint('tp_kafka-consumer-second_polled')
    def second_consumer_polled(_data):
        logging.info('Second consumer polled')

    @testpoint('tp_error_kafka-consumer-second')
    def second_consumer_failed(_data):
        logging.warning('Second consumer failed')

    await service_client.enable_testpoints()

    await start_consumers(service_client, [CONSUMERS[1]])

    await second_consumer_subscribed.wait_call()
    await second_consumer_subscribed.wait_call()

    await kafka_producer.produce(TOPIC, 'key-1', 'message-1', 0)
    await kafka_producer.produce(TOPIC, 'key-2', 'message-2', 1)

    await second_consumer_received.wait_call()
    await second_consumer_received.wait_call()

    second_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[1],
    )
    assert len(second_consumer_messages) == 2

    await kafka_producer.produce(TOPIC, 'key-3', MESSAGE_TO_FAIL, 0)
    await second_consumer_polled.wait_call()
    await second_consumer_failed.wait_call()

    await make_non_faulty(service_client, CONSUMERS[1])

    await second_consumer_received.wait_call()

    second_consumer_messages = await get_consumed_messages(
        service_client, CONSUMERS[1],
    )
    assert len(second_consumer_messages) == 1

    assert ['key-3'] == parse_message_keys(second_consumer_messages)

    await stop_consumers(service_client, [CONSUMERS[1]])
