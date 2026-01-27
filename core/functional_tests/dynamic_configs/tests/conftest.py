from typing import Any

import pytest

pytest_plugins = ['pytest_userver.plugins.core']


# For test_fixtures.py
@pytest.fixture(scope='session')
def dynamic_config_fallback_patch() -> dict[str, Any]:
    return {'HTTP_CLIENT_CONNECTION_POOL_SIZE': 777}
