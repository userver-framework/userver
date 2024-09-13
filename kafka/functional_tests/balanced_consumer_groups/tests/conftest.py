import asyncio
import json
import logging
import os

import common
from confluent_kafka import Producer
import pytest

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
def service_env():
    single_setting = {
        'brokers': os.getenv('KAFKA_RECIPE_BROKER_LIST'),
        'username': '',
        'password': '',
    }
    logging.info(f'Brokers are: {single_setting["brokers"]}')

    secdist_config = {
        'kafka_settings': {
            'kafka-consumer-first': single_setting,
            'kafka-consumer-second': single_setting,
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}


def _callback(err, msg) -> None:
    if err is not None:
        logging.error(
            f'Failed to deliver message to topic {msg.topic()}: {str(err)}',
        )
    else:
        logging.info(
            f'Message produced to topic {msg.topic()} with key {msg.key()} and partition {msg.partition()}',
        )


@pytest.fixture(name='kafka_producer', scope='session')
def kafka_producer():
    class Wrapper:
        def __init__(self):
            self.producer = Producer(common.get_kafka_conf())

        async def produce(
            self, topic, key, value, partition, callback=_callback,
        ) -> None:
            self.producer.produce(
                topic,
                value=value,
                key=key,
                partition=partition,
                on_delivery=callback,
            )
            self.producer.flush()

    return Wrapper()


@pytest.fixture(name='stop_consumers', autouse=True)
async def stop_consumers(service_client):
    yield

    logging.debug('Stopping consumers after test...')

    await asyncio.sleep(1.0)  # wait until messages are consumed
    for consumer in common.CONSUMERS:  # clear consumed messages, if exists
        await common.get_consumed_messages(service_client, consumer)

    # guarantee the consumers to be stopped after each test
    await common.stop_consumers(service_client)

    logging.debug('All consumers are stopped')
