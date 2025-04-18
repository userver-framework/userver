import json
import os

import pytest

pytest_plugins = ['pytest_userver.plugins.redis']

os.environ['TESTSUITE_REDIS_HOSTNAME'] = 'localhost'


@pytest.fixture(scope='session')
def service_env(redis_sentinels):
    secdist_config = {
        'redis_settings': {
            'redis-sentinel-0': {
                'password': '',
                'sentinels': redis_sentinels,
                'shards': [{'name': 'test_master1'}],
            },
            'redis-sentinel-1': {
                'password': '',
                'sentinels': redis_sentinels,
                'database_index': 1,
                'shards': [{'name': 'test_master1'}],
            },
            'redis-sentinel-2': {
                'password': '',
                'sentinels': redis_sentinels,
                'database_index': 2,
                'shards': [{'name': 'test_master1'}],
            },
        },
    }
    return {'SECDIST_CONFIG': json.dumps(secdist_config)}
