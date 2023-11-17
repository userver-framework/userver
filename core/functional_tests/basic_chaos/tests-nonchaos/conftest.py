import typing

import pytest

pytest_plugins = ['pytest_userver.plugins.core']


# For dynamic_configs/test_fixtures.py
@pytest.fixture(scope='session')
def dynamic_config_fallback_patch() -> typing.Dict[str, typing.Any]:
    return {'HTTP_CLIENT_CONNECTION_POOL_SIZE': 777}
