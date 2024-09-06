import json
import typing

import pytest

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
def userver_testsuite_middleware_enabled():
    return False


# Overriding a built-in userver configs service mock
@pytest.fixture
def mock_configs_service(mockserver) -> None:
    @mockserver.json_handler('/configs-service/configs/values')
    def _mock_configs(_request):
        return mockserver.make_response(
            'Emulating a downed dynconf service', 500,
        )

    @mockserver.json_handler('/configs-service/configs/status')
    def _mock_configs_status(_request):
        return mockserver.make_response(
            'Emulating a downed dynconf service', 500,
        )


# Overriding a built-in userver fixture
@pytest.fixture(scope='session')
def config_service_defaults():
    return {}


# TODO also test that the service uses configs from the cache.
_CONFIGS_CACHE: typing.Dict[str, typing.Any] = {}


# Overriding a built-in userver config hook
@pytest.fixture(name='userver_config_dynconf_cache', scope='session')
def _userver_config_dynconf_cache(
        userver_config_dynconf_cache, service_tmpdir,
):
    def patch_config(config_yaml, config_vars) -> None:
        userver_config_dynconf_cache(config_yaml, config_vars)

        cache_path = service_tmpdir / 'configs' / 'config_cache.json'
        cache_path.parent.mkdir(parents=True, exist_ok=True)
        with open(cache_path, 'w') as cache_file:
            json.dump(_CONFIGS_CACHE, cache_file)

    return patch_config
