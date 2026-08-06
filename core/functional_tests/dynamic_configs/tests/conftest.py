from typing import Any

import pytest

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(name='_userver_config_dumps', scope='session')
def _userver_config_dumps_fixture(_userver_config_dumps):
    def patch_config(config_yaml, config_vars) -> None:
        _userver_config_dumps(config_yaml, config_vars)
        config_vars['userver-dumps-periodic'] = True

    return patch_config


# For test_fixtures.py
@pytest.fixture(scope='session')
def dynamic_config_fallback_patch() -> dict[str, Any]:
    return {'HTTP_CLIENT_CONNECTION_POOL_SIZE': 777}
