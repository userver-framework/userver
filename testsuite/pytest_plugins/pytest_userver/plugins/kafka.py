"""
Plugin that imports the required fixtures to start the broker.
"""

import logging
import os
from typing import List

import pytest

from testsuite.databases.kafka.classes import BootstrapServers

pytest_plugins = [
    'testsuite.databases.kafka.pytest_plugin',
    'pytest_userver.plugins.core',
]


# @cond


@pytest.fixture(scope='session')
def _patched_bootstrap_servers_internal() -> BootstrapServers:
    """Used for internal testing purposes"""

    brokers_list = os.getenv('KAFKA_RECIPE_BROKER_LIST')
    if brokers_list:
        return brokers_list.split(',')

    return []


# @endcond


@pytest.fixture(scope='session')
def kafka_components() -> List[str]:
    """
    Should contain manually listed names of kafka producer and consumer
    components.

    @ingroup userver_testsuite_fixtures
    """

    return []


@pytest.fixture(scope='session')
def kafka_secdist(_bootstrap_servers, kafka_components) -> dict:
    """
    Automatically generates secdist config from user static config.
    `_bootstrap_servers` is testsuite's fixture that determines current
    bootstrap servers list depends on Kafka testsuite plugin's settings.

    @snippet samples/kafka_service/testsuite/conftest.py  Kafka service sample - secdist

    @ingroup userver_testsuite_fixtures
    """

    single_setting = {
        'brokers': _bootstrap_servers,
        'username': '',
        'password': '',
    }
    logging.info(f'Kafka brokers are: {single_setting["brokers"]}')

    return {
        'kafka_settings': {component_name: single_setting for component_name in kafka_components},
    }
