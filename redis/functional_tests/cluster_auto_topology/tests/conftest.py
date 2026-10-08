import json

import pytest

pytest_plugins = [
    'pytest_userver.plugins.redis',
    'pytest_userver.plugins.dynamic_config',
    'taxi.uservices.userver.redis.functional_tests.pytest_redis_cluster_topology_plugin.pytest_plugin',
]


@pytest.fixture(scope='session')
def service_env(redis_cluster_proxy_ports, redis_cluster_topology_session):
    cluster_hosts = []
    cluster_shards = []
    for index, port in enumerate(redis_cluster_proxy_ports):
        cluster_hosts.append({'host': '127.0.0.1', 'port': port})
    for index in range(3):
        cluster_shards.append({'name': f'shard{index}'})

    secdist_config = {
        'redis_settings': {
            'redis-cluster': {
                'password': '',
                'sentinels': cluster_hosts,
                'shards': cluster_shards,
            },
            'redis-cluster2': {
                'password': '',
                'sentinels': cluster_hosts,
                'shards': cluster_shards,
            },
            'redis-cluster3': {
                'password': '',
                'sentinels': cluster_hosts,
                'shards': cluster_shards,
            },
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}


@pytest.fixture(scope='session')
def redis_failover_timeouts():
    def patch_config(config, _config_vars):
        testsuite_support = config['components_manager']['components']['testsuite-support']
        # Use driver timeouts so one command cannot exhaust the failover polling deadline.
        disabled_timeout_override = '0s'
        testsuite_support['testsuite-redis-timeout-single'] = disabled_timeout_override
        testsuite_support['testsuite-redis-timeout-all'] = disabled_timeout_override

    return patch_config
