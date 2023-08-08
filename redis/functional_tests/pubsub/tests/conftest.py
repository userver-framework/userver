import json

import pytest


pytest_plugins = ['pytest_userver.plugins.redis']


@pytest.fixture(scope='session')
def service_env(redis_sentinels, redis_cluster_sentinels):
    cluster_shards = []
    cluster_primary_count = len(redis_cluster_sentinels) // 2
    for index in range(cluster_primary_count):
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
                'shards': [{'name': 'test_master0'}],
            },
        },
    }
    return {'SECDIST_CONFIG': json.dumps(secdist_config)}
