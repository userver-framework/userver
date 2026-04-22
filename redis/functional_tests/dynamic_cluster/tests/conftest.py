import json

import pytest

pytest_plugins = [
    'pytest_userver.plugins.redis',
    'pytest_userver.plugins.dynamic_config',
]


@pytest.fixture(scope='session')
def service_env():
    return {'SECDIST_CONFIG': json.dumps({})}
