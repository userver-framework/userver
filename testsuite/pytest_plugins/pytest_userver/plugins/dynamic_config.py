"""
Supply dynamic configs for the service in testsuite.
"""

# pylint: disable=redefined-outer-name
import dataclasses
import json
import pathlib
from typing import Callable
from typing import Iterable
from typing import Optional

import pytest

from pytest_userver import dynconf

USERVER_CONFIG_HOOKS = [
    'userver_config_dynconf_cache',
    'userver_config_dynconf_fallback',
    'userver_config_dynconf_url',
]
USERVER_CACHE_CONTROL_HOOKS = {
    'dynamic-config-client-updater': '_userver_dynconfig_cache_control',
}


@pytest.fixture
def dynamic_config(
    request,
    search_path,
    object_substitute,
    cache_invalidation_state,
    _dynamic_config_defaults_storage,
    config_service_defaults,
    dynamic_config_changelog,
    _dynconf_load_json_cached,
    dynconf_cache_names,
) -> dynconf.DynamicConfig:
    """
    Fixture that allows to control dynamic config values used by the service.

    Example:

    @snippet core/functional_tests/basic_chaos/tests-nonchaos/handlers/test_log_request_headers.py dynamic_config usage

    Example with @ref kill_switches "kill switches":

    @snippet core/functional_tests/dynamic_configs/tests/test_examples.py dynamic_config usage with kill switches

    HTTP and gRPC client requests call `update_server_state` automatically before each request.

    For main dynamic config documentation:

    @see @ref dynamic_config_testsuite

    See also other related fixtures:
    * @ref pytest_userver.plugins.dynamic_config.dynamic_config "config_service_defaults"
    * @ref pytest_userver.plugins.dynamic_config.dynamic_config "dynamic_config_fallback_patch"
    * @ref pytest_userver.plugins.dynamic_config.dynamic_config "mock_configs_service"

    @ingroup userver_testsuite_fixtures
    """
    config = dynconf.DynamicConfig(
        initial_values=config_service_defaults,
        defaults=_dynamic_config_defaults_storage.snapshot,
        config_cache_components=dynconf_cache_names,
        cache_invalidation_state=cache_invalidation_state,
        changelog=dynamic_config_changelog,
    )

    with dynamic_config_changelog.rollback(config_service_defaults):
        updates = {}
        for path in reversed(list(search_path('config.json'))):
            values = _dynconf_load_json_cached(path)
            updates.update(values)
        for marker in request.node.iter_markers('config'):
            value_update_kwargs = {
                key: value for key, value in marker.kwargs.items() if value is not dynconf.USE_STATIC_DEFAULT
            }
            value_updates_json = object_substitute(value_update_kwargs)
            updates.update(value_updates_json)
        config.set_values_unsafe(updates)

        kill_switches_disabled = []
        for marker in request.node.iter_markers('config'):
            kill_switches_disabled.extend(
                key for key, value in marker.kwargs.items() if value is dynconf.USE_STATIC_DEFAULT
            )
        config.switch_to_static_default(*kill_switches_disabled)

        yield config


# @cond


def pytest_configure(config):
    config.addinivalue_line(
        'markers',
        'config: per-test dynamic config values',
    )
    config.addinivalue_line(
        'markers',
        'disable_config_check: disable config mark keys check',
    )


# @endcond


@pytest.fixture(scope='session')
def dynconf_cache_names() -> Iterable[str]:
    return tuple(USERVER_CACHE_CONTROL_HOOKS.keys())


@pytest.fixture(scope='session')
def _dynconf_json_cache():
    return {}


@pytest.fixture
def _dynconf_load_json_cached(json_loads, _dynconf_json_cache):
    def load(path: pathlib.Path):
        if path not in _dynconf_json_cache:
            _dynconf_json_cache[path] = json_loads(path.read_text())
        return _dynconf_json_cache[path]

    return load


@pytest.fixture
def taxi_config(dynamic_config) -> dynconf.DynamicConfig:
    """
    Deprecated, use `dynamic_config` instead.
    """
    return dynamic_config


@pytest.fixture(scope='session')
def dynamic_config_fallback_patch() -> dynconf.ConfigValuesDict:
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
    config_fallback_path,
    dynamic_config_fallback_patch,
) -> dynconf.ConfigValuesDict:
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
    if not config_fallback_path:
        return dynamic_config_fallback_patch

    if pathlib.Path(config_fallback_path).exists():
        with open(config_fallback_path, 'r', encoding='utf-8') as file:
            fallback = json.load(file)
        fallback.update(dynamic_config_fallback_patch)
        return fallback

    raise RuntimeError(
        'Invalid path specified in config_fallback_path fixture. '
        'Probably invalid path was passed in --config-fallback pytest option.',
    )


@dataclasses.dataclass(frozen=False)
class _ConfigDefaults:
    snapshot: Optional[dynconf.ConfigValuesDict]

    async def update(self, client, dynamic_config) -> None:
        if self.snapshot is None:
            defaults = await client.get_dynamic_config_defaults()
            if not isinstance(defaults, dict):
                raise dynconf.InvalidDefaultsError()
            self.snapshot = defaults
            # pylint:disable=protected-access
            dynamic_config._defaults = defaults


# config_service_defaults fetches the dynamic config overrides, e.g. specified
# in the json file, then userver_config_dynconf_fallback forwards them
# to the service so that it has the correct dynamic config defaults.
#
# Independently of that, it is useful to have values for all configs, even
# unspecified in tests, on the testsuite side. For that, we ask the service
# for the dynamic config defaults after it's launched. It's enough to update
# defaults once per service launch.
@pytest.fixture(scope='session')
def _dynamic_config_defaults_storage() -> _ConfigDefaults:
    return _ConfigDefaults(snapshot=None)


@pytest.fixture(scope='session')
def userver_config_dynconf_cache(service_tmpdir):
    """
    Returns a function that adjusts the static configuration file for
    the testsuite.
    Sets `dynamic-config.fs-cache-path` to a file that is reset after the tests
    to avoid leaking dynamic config values between test sessions.

    @ingroup userver_testsuite_fixtures
    """

    def patch_config(config, _config_vars) -> None:
        components = config['components_manager']['components']
        dynamic_config_component = components.get('dynamic-config', None) or {}
        if dynamic_config_component.get('fs-cache-path', '') == '':
            return

        cache_path = service_tmpdir / 'configs' / 'config_cache.json'

        if cache_path.is_file():
            # To avoid leaking dynamic config values between test sessions
            cache_path.unlink()

        dynamic_config_component['fs-cache-path'] = str(cache_path)

    return patch_config


@pytest.fixture(scope='session')
def userver_config_dynconf_fallback(config_service_defaults):
    """
    Returns a function that adjusts the static configuration file for
    the testsuite.
    Removes `dynamic-config.defaults-path`.
    Updates `dynamic-config.defaults` with `config_service_defaults`.

    @ingroup userver_testsuite_fixtures
    """

    def extract_defaults_dict(component_config, config_vars) -> dict:
        defaults_field = component_config.get('defaults', None) or {}
        if isinstance(defaults_field, dict):
            return defaults_field
        elif isinstance(defaults_field, str):
            if defaults_field.startswith('$'):
                return config_vars.get(defaults_field[1:], {})
        assert False, f'Unexpected static config option `dynamic-config.defaults`: {defaults_field!r}'

    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        if components.get('dynamic-config', None) is None:
            components['dynamic-config'] = {}
        dynconf_component = components['dynamic-config']

        dynconf_component.pop('defaults-path', None)
        defaults = dict(
            extract_defaults_dict(dynconf_component, config_vars),
            **config_service_defaults,
        )
        dynconf_component['defaults'] = defaults

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
            client['append-path-to-url'] = True

    return _patch_config


# @cond


# TODO publish dynconf._Changelog and document how to use it in custom config service
#  mocks.
@pytest.fixture(scope='session')
def dynamic_config_changelog() -> dynconf._Changelog:
    return dynconf._Changelog()


# @endcond


@pytest.fixture
def mock_configs_service(
    mockserver,
    dynamic_config: dynconf.DynamicConfig,
    dynamic_config_changelog: dynconf._Changelog,
) -> None:
    """
    Adds a mockserver handler that forwards dynamic_config to service's
    `dynamic-config-client` component.

    @ingroup userver_testsuite_fixtures
    """

    @mockserver.json_handler('/configs-service/configs/values')
    def _mock_configs(request):
        updates = dynamic_config_changelog.get_updated_since(
            dynconf._create_config_dict(
                dynamic_config.get_values_unsafe(),
                dynamic_config.get_kill_switches_disabled_unsafe(),
            ),
            request.json.get('updated_since', ''),
            request.json.get('ids'),
        )
        response = {'configs': updates.values, 'updated_at': updates.timestamp}
        if updates.removed:
            response['removed'] = updates.removed
        if updates.kill_switches_disabled:
            response['kill_switches_disabled'] = updates.kill_switches_disabled
        return response

    @mockserver.json_handler('/configs-service/configs/status')
    def _mock_configs_status(_request):
        return {
            'updated_at': dynamic_config_changelog.timestamp.strftime(
                '%Y-%m-%dT%H:%M:%SZ',
            ),
        }


@pytest.fixture
def _userver_dynconfig_cache_control(dynamic_config_changelog: dynconf._Changelog):
    def cache_control(updater, timestamp):
        entry = dynamic_config_changelog.commit()
        if entry.timestamp == timestamp:
            updater.exclude()
        else:
            updater.incremental()
        return entry.timestamp

    return cache_control


_CHECK_CONFIG_ERROR = (
    'Your are trying to override config value using '
    '@pytest.mark.config({}) '
    'that does not seem to be used by your service.\n\n'
    'In case you really need to disable this check please add the '
    'following mark to your testcase:\n\n'
    '@pytest.mark.disable_config_check'
)


# Should be invoked after _dynamic_config_defaults_storage is filled.
@pytest.fixture
def _check_config_marks(
    request,
    _dynamic_config_defaults_storage,
) -> Callable[[], None]:
    def check():
        config_defaults = _dynamic_config_defaults_storage.snapshot
        assert config_defaults is not None

        if request.node.get_closest_marker('disable_config_check'):
            return

        unknown_configs = [
            key for marker in request.node.iter_markers('config') for key in marker.kwargs if key not in config_defaults
        ]

        if unknown_configs:
            message = _CHECK_CONFIG_ERROR.format(
                ', '.join(f'{key}=...' for key in sorted(unknown_configs)),
            )
            raise dynconf.UnknownConfigError(message)

    return check
