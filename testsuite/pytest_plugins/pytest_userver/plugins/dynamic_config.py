"""
Supply dynamic configs for the service in testsuite.
"""

import datetime
import json
import pathlib
import typing

import pytest

USERVER_CONFIG_HOOKS = [
    'userver_config_dynconf_fallback',
    'userver_config_dynconf_url',
]


class BaseError(Exception):
    """Base class for exceptions from this module"""


class DynamicConfigNotFoundError(BaseError):
    """Config parameter was not found and no default was provided"""


class DynamicConfig:
    """Simple dynamic config backend."""

    def __init__(self):
        self._values = {}

    def set_values(self, values):
        self._values.update(values)

    def get_values(self):
        return self._values.copy()

    def remove_values(self, keys):
        for key in keys:
            if key not in self._values:
                raise DynamicConfigNotFoundError(
                    f'param "{key}" is not found in config',
                )
            self._values.pop(key)

    def set(self, **values):
        self.set_values(values)

    def get(self, key, default=None):
        if key not in self._values:
            if default is not None:
                return default
            raise DynamicConfigNotFoundError(
                'param "%s" is not found in config' % (key,),
            )
        return self._values[key]

    def remove(self, key):
        return self.remove_values([key])


@pytest.fixture
def dynamic_config(
        request,
        search_path,
        load_json,
        object_substitute,
        config_service_defaults,
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
    config = DynamicConfig()
    all_values = config_service_defaults.copy()
    for path in reversed(list(search_path('config.json'))):
        values = load_json(path)
        all_values.update(values)
    for marker in request.node.iter_markers('config'):
        marker_json = object_substitute(marker.kwargs)
        all_values.update(marker_json)
    config.set_values(all_values)
    return config


@pytest.fixture
def taxi_config(dynamic_config) -> DynamicConfig:
    """
    Deprecated, use `dynamic_config` instead.
    """
    return dynamic_config


@pytest.fixture(scope='session')
def config_service_defaults(
        config_fallback_path,
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
            return json.load(file)

    raise RuntimeError(
        'Either provide the path to dynamic config defaults file using '
        '--config-fallback pytest option, or override '
        f'{config_service_defaults.__name__} fixture to provide custom '
        'dynamic config loading behavior.',
    )


@pytest.fixture(scope='session')
def userver_config_dynconf_fallback(pytestconfig, config_fallback_path):
    def patch_config(config_yaml, _config_vars):
        components = config_yaml['components_manager']['components']
        for component_name in (
                'dynamic-config-fallbacks',
                'dynamic-config-client-updater',
        ):
            if component_name not in components:
                continue
            component = components[component_name]
            if not config_fallback_path:
                pytest.fail('Please run with --config-fallback=...')
            component['fallback-path'] = str(config_fallback_path)

    return patch_config


@pytest.fixture(scope='session')
def userver_config_dynconf_url(mockserver_info):
    def patch_config(config, _config_vars) -> None:
        components = config['components_manager']['components']
        client = components.get('dynamic-config-client', None)
        if client:
            client['config-url'] = mockserver_info.url('configs-service')

    return patch_config


@pytest.fixture
def mock_configs_service(mockserver, dynamic_config) -> None:
    """
    Adds a mockserver handler that forwards dynamic_config to service's
    dynamic-config-client component.
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
