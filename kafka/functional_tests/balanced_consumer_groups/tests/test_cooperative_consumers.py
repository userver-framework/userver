import common

TOPIC = 'test-cooperative-topic'

CONSUMERS = ['kafka-consumer-cooperative-first', 'kafka-consumer-cooperative-second']


async def test_cooperative_consumers(service_client, testpoint, kafka_producer):
    @testpoint('tp_kafka-consumer-cooperative-first')
    def first_consumer_received(_data):
        pass

    @testpoint('tp_kafka-consumer-cooperative-second')
    def second_consumer_received(_data):
        pass

    await service_client.enable_testpoints()

    await common.start_consumers(service_client, CONSUMERS)

    await kafka_producer.send(TOPIC, 'key-1', 'message-1', 0)
    await kafka_producer.send(TOPIC, 'key-2', 'message-2', 1)

    await first_consumer_received.wait_call()
    await second_consumer_received.wait_call()

    await common.stop_consumers(service_client, [CONSUMERS[1]])

    await kafka_producer.send(TOPIC, 'key-3', 'message-3', 0)
    await kafka_producer.send(TOPIC, 'key-4', 'message-4', 1)

    await first_consumer_received.wait_call()
    await first_consumer_received.wait_call()

    await common.stop_consumers(service_client, [CONSUMERS[0]])
