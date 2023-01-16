import json

import pytest

pytest_plugins = ['pytest_userver.plugins.redis']

SECDIST_CONFIG = {
    'redis_settings': {
        'metrics_test': {
            'password': '',
            'sentinels': [{'host': 'localhost', 'port': 26379}],
            'shards': [{'name': 'test_master0'}],
        },
    },
}


@pytest.fixture(scope='session')
def service_env():
    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}
