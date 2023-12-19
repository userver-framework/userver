import json
import os

import pytest


pytest_plugins = ['pytest_userver.plugins.redis']

os.environ['TESTSUITE_REDIS_HOSTNAME'] = 'localhost'


@pytest.fixture(scope='session')
def service_env(redis_sentinels, redis_cluster_nodes, redis_cluster_replicas):
    cluster_shards = [
        {'name': f'shard{idx}'}
        for idx in range(
            len(redis_cluster_nodes) // (redis_cluster_replicas + 1),
        )
    ]

    secdist_config = {
        'redis_settings': {
            'redis-cluster': {
                'password': '',
                'sentinels': redis_cluster_nodes,
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
