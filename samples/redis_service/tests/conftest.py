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


@pytest.fixture(scope='session')
def test_service_env():
    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}


@pytest.fixture
def client_deps(redis_store):
    pass
