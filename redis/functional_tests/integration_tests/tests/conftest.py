import json

import pytest


pytest_plugins = [
    'pytest_userver.plugins.base',
    'pytest_userver.plugins.service_client',
    'pytest_userver.plugins.config',
    'pytest_userver.plugins.service',
    'taxi.integration_testing.pytest_plugin',
]


@pytest.fixture
def client_deps(
        service_config_yaml,
        _redis_service_settings,
        redis_docker_service,
        redis_slave_0_docker_service,
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
