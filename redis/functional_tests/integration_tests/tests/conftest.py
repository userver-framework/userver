import json

import pytest


pytest_plugins = ['pytest_userver.plugins.redis']


@pytest.fixture(scope='session')
def service_env(redis_sentinels, redis_cluster_sentinels):
    cluster_shards = []
    for index, _ in enumerate(redis_cluster_sentinels):
        cluster_shards.append({'name': f'shard{index}'})

    secdist_config = {
        'redis_settings': {
            'redis-cluster': {
                'password': '',
                'sentinels': redis_cluster_sentinels,
                'shards': cluster_shards,
            },
            'redis-sentinel': {
                'password': '',
                'sentinels': redis_sentinels,
                'shards': [{'name': 'test_master1'}],
            },
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}
