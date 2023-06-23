import json

import pytest


pytest_plugins = [
    'pytest_userver.plugins.base',
    'pytest_userver.plugins.caches',
    'pytest_userver.plugins.config',
    'pytest_userver.plugins.dumps',
    'pytest_userver.plugins.dynamic_config',
    'pytest_userver.plugins.log_capture',
    'pytest_userver.plugins.service',
    'pytest_userver.plugins.service_client',
    'pytest_userver.plugins.testpoint',
    'taxi.integration_testing.pytest_plugin',
    'taxi.uservices.testsuite.integration_testing.pytest_plugin',
]


@pytest.fixture(scope='session')
def dynamic_config_fallback_patch():
    return {'REDIS_CLUSTER_AUTOTOPOLOGY_ENABLED': True}


@pytest.fixture(scope='session')
def service_env(redis_sentinel_services, redis_cluster_topology_services):
    cluster_hosts = []
    cluster_shards = []
    for index, _ in enumerate(redis_cluster_topology_services.masters):
        cluster_hosts.append(
            {
                'host': redis_cluster_topology_services.master_host(index),
                'port': 6379,
            },
        )
        cluster_shards.append({'name': f'shard{index}'})

    secdist_config = {
        'redis_settings': {
            'redis-cluster': {
                'password': '',
                'sentinels': cluster_hosts,
                'shards': cluster_shards,
            },
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}
