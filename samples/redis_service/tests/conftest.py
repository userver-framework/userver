import json

import pytest


SECDIST_CONFIG = {
    'redis_settings': {
        'taxi-tmp': {
            'password': '',
            'sentinels': [{'host': 'localhost', 'port': 26379}],
            'shards': [{'name': 'test_master0'}],
        },
    },
}


# /// [service_env]
@pytest.fixture(scope='session')
def service_env():
    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}
    # /// [service_env]


@pytest.fixture
def client_deps(redis_store):
    pass
