import asyncio
import logging

import common
import pytest

pytest_plugins = ['pytest_userver.plugins.kafka']


@pytest.fixture(scope='session')
def kafka_local(bootstrap_servers):
    return bootstrap_servers


@pytest.fixture(scope='session')
def service_env(kafka_secdist):
    return {'SECDIST_CONFIG': kafka_secdist}


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
