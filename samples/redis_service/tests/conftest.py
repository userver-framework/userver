import json

import pytest

pytest_plugins = [
    'pytest_userver.plugins',
    'pytest_userver.plugins.samples',
    # Database related plugins
    'testsuite.databases.redis.pytest_plugin',
]

# /// [service_env value]
SECDIST_CONFIG = {
    'redis_settings': {
        'taxi-tmp': {
            'password': '',
            'sentinels': [{'host': 'localhost', 'port': 26379}],
            'shards': [{'name': 'test_master0'}],
        },
    },
}
# /// [service_env value]

# /// [service_env]
@pytest.fixture(scope='session')
def service_env():
    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}
    # /// [service_env]


# /// [client_deps]
@pytest.fixture
def client_deps(redis_store):
    pass
    # /// [client_deps]
