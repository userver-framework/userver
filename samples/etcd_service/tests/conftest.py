import json

import pytest

pytest_plugins = [
    'pytest_userver.plugins',
    'pytest_userver.plugins.samples',
]

# /// [service_env value]
SECDIST_CONFIG = {
    
}
# /// [service_env value]

# /// [service_env]
@pytest.fixture(scope='session')
def service_env():
    return {'SECDIST_CONFIG': json.dumps(SECDIST_CONFIG)}
    # /// [service_env]
