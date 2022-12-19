import json

import pytest


pytest_plugins = [
    'pytest_userver.plugins.base',
    'pytest_userver.plugins.samples',
    'pytest_userver.plugins.service_client',
    'pytest_userver.plugins.config',
    'pytest_userver.plugins.service',
    'taxi.integration_testing.pytest_plugin',
]


@pytest.fixture
def client_deps(
        service_config_yaml,
        _redis_service_settings,
        docker_service_redis,
        docker_service_redis_slave_0,
):
    config = service_config_yaml['components_manager']['components'][
        'default-secdist-provider'
    ]['config']
    with open(config, 'r') as ffi:
        contents = json.load(ffi)

    contents['redis_settings']['redis1']['sentinels'] = [
        {
            'host': _redis_service_settings.host,
            'port': _redis_service_settings.sentinel_port,
        },
    ]

    with open(config, 'w') as ffo:
        json.dump(contents, ffo, indent='  ')
