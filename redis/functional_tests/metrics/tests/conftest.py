import json

import pytest

pytest_plugins = ['pytest_userver.plugins.redis']


@pytest.fixture(scope='session')
def dynamic_config_fallback_patch():
    return {
        'REDIS_METRICS_SETTINGS': {
            'command-timings-enabled': True,
            'reply-sizes-enabled': True,
            'request-sizes-enabled': True,
            'timings-enabled': True,
        },
        'REDIS_PUBSUB_METRICS_SETTINGS': {'per-shard-stats-enabled': True},
    }


@pytest.fixture(scope='session')
def service_env(redis_sentinels):
    secdist_config = {
        'redis_settings': {
            'metrics_test': {
                'password': '',
                'sentinels': redis_sentinels,
                'shards': [{'name': 'test_master0'}],
            },
        },
    }
    return {'SECDIST_CONFIG': json.dumps(secdist_config)}
