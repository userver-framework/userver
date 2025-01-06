# /// [redis setup]
import json

import pytest

pytest_plugins = ['pytest_userver.plugins.redis']
# /// [redis setup]


# /// [service_env]
@pytest.fixture(scope='session')
def service_env(redis_sentinels):
    secdist_config = {
        'redis_settings': {
            'taxi-tmp': {
                'password': '',
                'sentinels': redis_sentinels,
                'shards': [{'name': 'test_master0'}],
            },
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}
    # /// [service_env]
