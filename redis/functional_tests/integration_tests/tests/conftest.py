import json
import os

import pytest

pytest_plugins = ['pytest_userver.plugins.redis']

os.environ['TESTSUITE_REDIS_HOSTNAME'] = 'localhost'


# /// [Sample pytest redis configuration]
@pytest.fixture(scope='session')
def service_env(redis_sentinels, redis_cluster_nodes, redis_cluster_replicas, redis_standalone_node):
    cluster_shards = [
        {'name': f'shard{idx}'}
        for idx in range(
            len(redis_cluster_nodes) // (redis_cluster_replicas + 1),
        )
    ]

    # All the `sentinels` accept host and port. For example `'sentinels': [{'host': '192.168.1.7','port': 6379}]`
    secdist_config = {
        'redis_settings': {
            'redis-cluster': {
                'password': '',
                'sentinels': redis_cluster_nodes,
                'shards': cluster_shards,  # For example, it can be [{"name": "shard0"}, {"name": "shard1"}]
            },
            'redis-sentinel': {
                'password': '',
                'sentinels': redis_sentinels,
                'database_index': 5,  # database index to use, 0 by default
                'shards': [{'name': 'test_master1'}],
            },
            'redis-standalone': {
                'password': '',
                'sentinels': [redis_standalone_node],
                'shards': [{'name': 'test_standalone_master0'}],
            },
        },
    }
    return {'SECDIST_CONFIG': json.dumps(secdist_config)}


@pytest.fixture
def extra_client_deps(redis_cluster_store, redis_standalone_store):
    # `redis_store` is autodetected by pytest_userver.plugins.service.auto_client_deps fixture
    pass
    # /// [Sample pytest redis configuration]
