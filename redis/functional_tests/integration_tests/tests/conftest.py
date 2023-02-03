import json

import pytest


pytest_plugins = [
    'pytest_userver.plugins.base',
    'pytest_userver.plugins.config',
    'pytest_userver.plugins.dynamic_config',
    'pytest_userver.plugins.service',
    'pytest_userver.plugins.service_client',
    'taxi.integration_testing.pytest_plugin',
]


@pytest.fixture(scope='session')
def service_env(_redis_sentinel_docker_service, _redis_cluster_docker_service):
    cluster_hosts = []
    cluster_shards = []
    for index, _ in enumerate(_redis_cluster_docker_service.masters):
        cluster_hosts.append(
            {
                'host': 'localhost',
                'port': _redis_cluster_docker_service.master_port(index),
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
            'redis-sentinel': {
                'password': '',
                'sentinels': [
                    {
                        'host': 'localhost',
                        'port': _redis_sentinel_docker_service.sentinel_port(),
                    },
                ],
                'shards': [{'name': 'shard0'}],
            },
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}
