import json
import logging
import os


import confluent_kafka
import pytest


pytest_plugins = ['pytest_userver.plugins.core']


# /// [Kafka service sample - secdist]
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
            'kafka-consumer': single_setting,
            'kafka-producer-first': single_setting,
            'kafka-producer-second': single_setting,
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}
    # /// [Kafka service sample - secdist]


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
            self.producer = confluent_kafka.Producer(
                {'bootstrap.servers': os.getenv('KAFKA_RECIPE_BROKER_LIST')},
            )

        async def produce(self, topic, key, value, callback=_callback):
            self.producer.produce(
                topic, value=value, key=key, on_delivery=callback,
            )
            self.producer.flush()

    return Wrapper()
