import json

import pytest


pytest_plugins = ['pytest_userver.plugins.rabbitmq']


SECDIST_CONFIG = {
    # /// [RabbitMQ service sample - secdist]
    # json
    'rabbitmq_settings': {
        'my-rabbit-alias': {
            'hosts': ['localhost'],
            'port': 8672,
            'login': 'guest',
            'password': 'guest',
            'vhost': '/',
        },
    },
    # /// [RabbitMQ service sample - secdist]
}


@pytest.fixture(scope='session')
def service_env():
    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}
