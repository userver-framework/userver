import copy
import json
import os

import pytest
import pytest_userver.service

pytest_plugins = ['pytest_userver.plugins.redis']

os.environ['TESTSUITE_REDIS_HOSTNAME'] = 'localhost'


@pytest.fixture(scope='session', params=['explicit', 'paired', 'separate'])
def redis_group_mode(request):
    return request.param


@pytest.fixture(scope='session')
def service_config_yaml(service_config_yaml, redis_group_mode):
    config = copy.deepcopy(service_config_yaml)
    if redis_group_mode != 'explicit':
        redis_config = config['components_manager']['components']['key-value-database']
        for group in redis_config['groups']:
            group['enable_subscriptions'] = False
            subscribe_group = {key: value for key, value in group.items() if key != 'enable_subscriptions'}
            if redis_group_mode == 'separate':
                subscribe_group['max_disconnect_time'] = 36
            redis_config['subscribe_groups'].append(subscribe_group)
    return config


@pytest.fixture(scope='session')
def empty_cluster_shards_config():
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        components['key-value-database']['groups'].append(
            {
                'config_name': 'redis-cluster-empty-shards',
                'db': 'redis-cluster-empty-shards',
                'sharding_strategy': 'RedisCluster',
            },
        )
        components['handler-cluster']['db'] = 'redis-cluster-empty-shards'

    return patch_config


@pytest.fixture(scope='session')
def service_env(redis_sentinels, redis_cluster_nodes, redis_cluster_replicas, redis_standalone_node):
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
            'redis-cluster-empty-shards': {
                'password': '',
                'sentinels': redis_cluster_nodes,
                'shards': [],
            },
            'redis-sentinel': {
                'password': '',
                'sentinels': redis_sentinels,
                'shards': [{'name': 'test_master0'}],
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
@pytest_userver.service.dependency
def _redis_extra_deps(redis_cluster_store, redis_standalone_store):
    pass
