import json
import os

import pytest
import logging


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
            'kafka-consumer': single_setting,
            'kafka-producer-first': single_setting,
            'kafka-producer-second': single_setting,
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}
