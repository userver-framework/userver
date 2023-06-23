"""
Supply dynamic configs for the service in testsuite.
"""

import copy
import datetime
import json
import pathlib
import typing

import pytest

from pytest_userver.plugins import caches

USERVER_CONFIG_HOOKS = [
    'userver_config_dynconf_cache',
    'userver_config_dynconf_fallback',
    'userver_config_dynconf_url',
]

_CONFIG_CACHES = ['dynamic-config-client-updater']


class BaseError(Exception):
    """Base class for exceptions from this module"""


class DynamicConfigNotFoundError(BaseError):
    """Config parameter was not found and no default was provided"""


class DynamicConfig:
    """Simple dynamic config backend."""

    def __init__(
            self,
            *,
            initial_values: typing.Dict[str, typing.Any],
            config_cache_components: typing.Iterable[str],
            cache_invalidation_state: caches.InvalidationState,
    ):
        self._values = copy.deepcopy(initial_values)
        self._cache_invalidation_state = cache_invalidation_state
        self._config_cache_components = config_cache_components

    def set_values(self, values):
        self._values.update(values)
        self._sync_with_service()

    def get_values(self):
        return self._values.copy()

    def remove_values(self, keys):
        extra_keys = set(keys).difference(self._values.keys())
        if extra_keys:
            raise DynamicConfigNotFoundError(
                f'Attempting to remove nonexistent configs: {extra_keys}',
            )
        for key in keys:
            self._values.pop(key)
        self._sync_with_service()

    def set(self, **values):
        self.set_values(values)

    def get(self, key, default=None):
        if key not in self._values:
            if default is not None:
                return default
            raise DynamicConfigNotFoundError(f'Config {key!r} is not found')
        return self._values[key]

    def remove(self, key):
        return self.remove_values([key])

    def _sync_with_service(self):
        self._cache_invalidation_state.invalidate(
            self._config_cache_components,
        )


@pytest.fixture
def dynamic_config(
        request,
        search_path,
        load_json,
        object_substitute,
        config_service_defaults,
        cache_invalidation_state,
) -> DynamicConfig:
    """
    Fixture that allows to control dynamic config values used by the service.

    After change to the config, be sure to call:
    @code
    await service_client.update_server_state()
    @endcode

    HTTP client requests call it automatically before each request.

    @ingroup userver_testsuite_fixtures
    """
    all_values = config_service_defaults.copy()
    for path in reversed(list(search_path('config.json'))):
        values = load_json(path)
        all_values.update(values)
    for marker in request.node.iter_markers('config'):
        marker_json = object_substitute(marker.kwargs)
        all_values.update(marker_json)
    return DynamicConfig(
        initial_values=all_values,
        config_cache_components=_CONFIG_CACHES,
        cache_invalidation_state=cache_invalidation_state,
    )


@pytest.fixture
def taxi_config(dynamic_config) -> DynamicConfig:
    """
    Deprecated, use `dynamic_config` instead.
    """
    return dynamic_config


@pytest.fixture(scope='session')
def dynamic_config_fallback_patch() -> typing.Dict[str, typing.Any]:
    """
    Override this fixture to replace some dynamic config values specifically
    for testsuite tests:

    @code
    @pytest.fixture(scope='session')
    def dynamic_config_fallback_patch():
        return {"MY_CONFIG_NAME": 42}
    @endcode

    @ingroup userver_testsuite_fixtures
    """
    return {}


@pytest.fixture(scope='session')
def config_service_defaults(
        config_fallback_path, dynamic_config_fallback_patch,
) -> typing.Dict[str, typing.Any]:
    """
    Fixture that returns default values for dynamic config. You may override
    it in your local conftest.py or fixture:

    @code
    @pytest.fixture(scope='session')
    def config_service_defaults():
        with open('defaults.json') as fp:
            return json.load(fp)
    @endcode

    @ingroup userver_testsuite_fixtures
    """
    if config_fallback_path and pathlib.Path(config_fallback_path).exists():
        with open(config_fallback_path, 'r', encoding='utf-8') as file:
            fallback = json.load(file)
        fallback.update(dynamic_config_fallback_patch)
        return fallback

    raise RuntimeError(
        'Either provide the path to dynamic config defaults file using '
        '--config-fallback pytest option, or override '
        f'{config_service_defaults.__name__} fixture to provide custom '
        'dynamic config loading behavior.',
    )


@pytest.fixture(scope='session')
def userver_config_dynconf_cache(service_tmpdir):
    def patch_config(config, _config_vars) -> None:
        components = config['components_manager']['components']
        dynamic_config_component = components.get('dynamic-config', {})
        if dynamic_config_component.get('fs-cache-path', '') == '':
            return

        cache_path = service_tmpdir / 'configs' / 'config_cache.json'

        if cache_path.is_file():
            # To avoid leaking dynamic config values between test sessions
            cache_path.unlink()

        dynamic_config_component['fs-cache-path'] = str(cache_path)

    return patch_config


_COMPONENTS_WITH_FALLBACK = {
    'dynamic-config-fallbacks',
    'dynamic-config-client-updater',
}


@pytest.fixture(scope='session')
def userver_config_dynconf_fallback(
        pytestconfig, config_service_defaults, service_tmpdir,
):
    """
    Returns a function that adjusts the static configuration file for
    the testsuite.
    Sets the `fallback-path` of the `dynamic-config-client-updater` and
    `dynamic-config-fallbacks` according to `config_service_defaults`.

    @ingroup userver_testsuite_fixtures
    """

    def _patch_config(config_yaml, _config_vars):
        components = config_yaml['components_manager']['components']
        if not (components.keys() & _COMPONENTS_WITH_FALLBACK):
            return

        fallback_path = (
            service_tmpdir / 'configs' / 'dynamic_config_fallback.json'
        )
        fallback_path.parent.mkdir(exist_ok=True)
        with open(fallback_path, 'w', encoding='utf-8') as file:
            json.dump(config_service_defaults, file)

        for component_name in _COMPONENTS_WITH_FALLBACK:
            if component_name not in components:
                continue
            component = components[component_name]
            component['fallback-path'] = str(fallback_path)

    return _patch_config


@pytest.fixture(scope='session')
def userver_config_dynconf_url(mockserver_info):
    """
    Returns a function that adjusts the static configuration file for
    the testsuite.
    Sets the `dynamic-config-client.config-url` to the value of mockserver
    configs-service, so that the
    @ref pytest_userver.plugins.dynamic_config.mock_configs_service
    "mock_configs_service" fixture could work.

    @ingroup userver_testsuite_fixtures
    """

    def _patch_config(config, _config_vars) -> None:
        components = config['components_manager']['components']
        client = components.get('dynamic-config-client', None)
        if client:
            client['config-url'] = mockserver_info.url('configs-service')

    return _patch_config


@pytest.fixture
def mock_configs_service(mockserver, dynamic_config) -> None:
    """
    Adds a mockserver handler that forwards dynamic_config to service's
    `dynamic-config-client` component.

    @ingroup userver_testsuite_fixtures
    """

    def service_timestamp():
        return datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%SZ')

    @mockserver.json_handler('/configs-service/configs/values')
    def _mock_configs(request):
        values = dynamic_config.get_values()
        if request.json.get('ids'):
            values = {
                name: values[name]
                for name in request.json['ids']
                if name in values
            }
        return {'configs': values, 'updated_at': service_timestamp()}

    @mockserver.json_handler('/configs-service/configs/status')
    def _mock_configs_status(_request):
        return {'updated_at': service_timestamp()}
