import json
import logging


import pytest


pytest_plugins = ['pytest_userver.plugins.kafka']

logger = logging.getLogger(__name__)


# /// [Kafka service sample - secdist]
@pytest.fixture(scope='session')
def service_env(kafka_secdist):
    return {'SECDIST_CONFIG': kafka_secdist}
    # /// [Kafka service sample - secdist]

