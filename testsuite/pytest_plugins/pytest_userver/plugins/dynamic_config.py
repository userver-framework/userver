"""
Supply dynamic configs for the service in testsuite.
"""

# pylint: disable=redefined-outer-name
import contextlib
import copy
import dataclasses
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
USERVER_CACHE_CONTROL_HOOKS = {
    'dynamic-config-client-updater': '_userver_dynconfig_cache_control',
}


class BaseError(Exception):
    """Base class for exceptions from this module"""


class DynamicConfigNotFoundError(BaseError):
    """Config parameter was not found and no default was provided"""


class DynamicConfigUninitialized(BaseError):
    """
    Calling `dynamic_config.get` before defaults are fetched from the service.
    Try adding a dependency on `service_client` in your fixture.
    """


class InvalidDefaultsError(BaseError):
    """Dynamic config defaults action returned invalid response"""


class UnknownConfigError(BaseError):
    """Invalid dynamic config name in @pytest.mark.config"""


ConfigDict = typing.Dict[str, typing.Any]


class RemoveKey:
    pass


class Missing:
    pass


@dataclasses.dataclass
class _ChangelogEntry:
    timestamp: str
    dirty_keys: typing.Set[str]
    state: ConfigDict
    prev_state: ConfigDict

    @classmethod
    def new(
            cls,
            *,
            previous: typing.Optional['_ChangelogEntry'],
            timestamp: str,
    ):
        if previous:
            prev_state = previous.state
        else:
            prev_state = {}
        return cls(
            timestamp=timestamp,
            dirty_keys=set(),
            state=prev_state.copy(),
            prev_state=prev_state,
        )

    @property
    def has_changes(self) -> bool:
        return bool(self.dirty_keys)

    def update(self, values: ConfigDict):
        for key, value in values.items():
            if value == self.prev_state.get(key, Missing):
                self.dirty_keys.discard(key)
            else:
                self.dirty_keys.add(key)
        self.state.update(values)


@dataclasses.dataclass(frozen=True)
class Updates:
    timestamp: str
    values: ConfigDict
    removed: typing.List[str]

    def is_empty(self) -> bool:
        return not self.values and not self.removed


class _Changelog:
    timestamp: datetime.datetime
    commited_entries: typing.List[_ChangelogEntry]
    staged_entry: _ChangelogEntry

    def __init__(self):
        self.timestamp = datetime.datetime.fromtimestamp(
            0, datetime.timezone.utc,
        )
        self.commited_entries = []
        self.staged_entry = _ChangelogEntry.new(
            timestamp=self.service_timestamp(), previous=None,
        )

    def service_timestamp(self) -> str:
        return self.timestamp.strftime('%Y-%m-%dT%H:%M:%SZ')

    def next_timestamp(self) -> str:
        self.timestamp += datetime.timedelta(seconds=1)
        return self.service_timestamp()

    def commit(self) -> _ChangelogEntry:
        """Commit staged changed if any and return last commited entry."""
        entry = self.staged_entry
        if entry.has_changes or not self.commited_entries:
            self.staged_entry = _ChangelogEntry.new(
                timestamp=self.next_timestamp(), previous=entry,
            )
            self.commited_entries.append(entry)
        return self.commited_entries[-1]

    def get_updated_since(
            self,
            values: ConfigDict,
            updated_since: str,
            ids: typing.Optional[typing.List[str]] = None,
    ) -> Updates:
        entry = self.commit()
        values, removed = self._get_updated_since(values, updated_since)
        if ids:
            values = {name: values[name] for name in ids if name in values}
            removed = [name for name in removed if name in ids]
        return Updates(
            timestamp=entry.timestamp, values=values, removed=removed,
        )

    def _get_updated_since(
            self, values: ConfigDict, updated_since: str,
    ) -> typing.Tuple[ConfigDict, typing.List[str]]:
        if not updated_since:
            return values, []
        dirty_keys = set()
        last_known_state = {}
        for entry in reversed(self.commited_entries):
            if entry.timestamp > updated_since:
                dirty_keys.update(entry.dirty_keys)
            else:
                if entry.timestamp == updated_since:
                    last_known_state = entry.state
                break
        # We don't want to send them again
        result = {}
        removed = []
        for key in dirty_keys:
            value = values.get(key, RemoveKey)
            if last_known_state.get(key, Missing) != value:
                if value is RemoveKey:
                    removed.append(key)
                else:
                    result[key] = value
        return result, removed

    def add_entries(self, values: ConfigDict):
        self.staged_entry.update(values)

    @contextlib.contextmanager
    def rollback(self, defaults: ConfigDict):
        try:
            yield
        finally:
            self._do_rollback(defaults)

    def _do_rollback(self, defaults: ConfigDict):
        if not self.commited_entries:
            return

        maybe_dirty = set()
        for entry in self.commited_entries:
            maybe_dirty.update(entry.dirty_keys)

        last = self.commited_entries[-1]
        last_state = last.state
        dirty_keys = set()
        reverted = {}
        for key in maybe_dirty:
            original = defaults.get(key, RemoveKey)
            if last_state[key] != original:
                dirty_keys.add(key)
            reverted[key] = original

        entry = _ChangelogEntry(
            timestamp=last.timestamp,
            state=last.state,
            dirty_keys=dirty_keys,
            prev_state={},
        )
        self.commited_entries = [entry]
        self.staged_entry = _ChangelogEntry(
            timestamp=self.staged_entry.timestamp,
            dirty_keys=dirty_keys.copy(),
            state=reverted,
            prev_state=entry.state,
        )


class DynamicConfig:
    """Simple dynamic config backend."""

    def __init__(
            self,
            *,
            initial_values: ConfigDict,
            defaults: typing.Optional[ConfigDict],
            config_cache_components: typing.Iterable[str],
            cache_invalidation_state: caches.InvalidationState,
            changelog: _Changelog,
    ):
        self._values = initial_values.copy()
        # Defaults are only there for convenience, to allow accessing them
        # in tests using dynamic_config.get. They are not sent to the service.
        self._defaults = defaults
        self._cache_invalidation_state = cache_invalidation_state
        self._config_cache_components = config_cache_components
        self._changelog = changelog

    def set_values(self, values: ConfigDict):
        self.set_values_unsafe(copy.deepcopy(values))

    def set_values_unsafe(self, values: ConfigDict):
        self._values.update(values)
        self._changelog.add_entries(values)
        self._sync_with_service()

    def set(self, **values):
        self.set_values(values)

    def get_values_unsafe(self) -> ConfigDict:
        return self._values

    def get(self, key: str, default: typing.Any = None) -> typing.Any:
        if key in self._values:
            return copy.deepcopy(self._values[key])
        if self._defaults is not None and key in self._defaults:
            return copy.deepcopy(self._defaults[key])
        if default is not None:
            return default
        if self._defaults is None:
            raise DynamicConfigUninitialized(
                f'Defaults for config {key!r} have not yet been fetched '
                'from the service. Options:\n'
                '1. add a dependency on service_client in your fixture;\n'
                '2. pass `default` parameter to `dynamic_config.get`',
            )
        raise DynamicConfigNotFoundError(f'Config {key!r} is not found')

    def remove_values(self, keys):
        extra_keys = set(keys).difference(self._values.keys())
        if extra_keys:
            raise DynamicConfigNotFoundError(
                f'Attempting to remove nonexistent configs: {extra_keys}',
            )
        for key in keys:
            self._values.pop(key)
        self._changelog.add_entries({key: RemoveKey for key in keys})
        self._sync_with_service()

    def remove(self, key):
        return self.remove_values([key])

    @contextlib.contextmanager
    def modify(self, key: str) -> typing.Any:
        value = self.get(key)
        yield value
        self.set_values({key: value})

    @contextlib.contextmanager
    def modify_many(
            self, *keys: typing.Tuple[str, ...],
    ) -> typing.Tuple[typing.Any, ...]:
        values = tuple(self.get(key) for key in keys)
        yield values
        self.set_values(dict(zip(keys, values)))

    def _sync_with_service(self):
        self._cache_invalidation_state.invalidate(
            self._config_cache_components,
        )


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
    config = DynamicConfig(
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
            marker_json = object_substitute(marker.kwargs)
            updates.update(marker_json)
        config.set_values_unsafe(updates)
        yield config


def pytest_configure(config):
    config.addinivalue_line(
        'markers', 'config: per-test dynamic config values',
    )
    config.addinivalue_line(
        'markers', 'disable_config_check: disable config mark keys check',
    )


@pytest.fixture(scope='session')
def dynconf_cache_names() -> typing.Iterable[str]:
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
def taxi_config(dynamic_config) -> DynamicConfig:
    """
    Deprecated, use `dynamic_config` instead.
    """
    return dynamic_config


@pytest.fixture(scope='session')
def dynamic_config_fallback_patch() -> ConfigDict:
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
) -> ConfigDict:
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
    snapshot: typing.Optional[ConfigDict]

    async def update(self, client, dynamic_config) -> None:
        if self.snapshot is None:
            defaults = await client.get_dynamic_config_defaults()
            if not isinstance(defaults, dict):
                raise InvalidDefaultsError()
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
@pytest.fixture(scope='package')
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
        assert False, (
            f'Unexpected static config option '
            f'`dynamic-config.defaults`: {defaults_field!r}'
        )

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


# TODO publish _Changelog and document how to use it in custom config service
#  mocks.
@pytest.fixture(scope='session')
def dynamic_config_changelog() -> _Changelog:
    return _Changelog()


@pytest.fixture
def mock_configs_service(
        mockserver,
        dynamic_config: DynamicConfig,
        dynamic_config_changelog: _Changelog,
) -> None:
    """
    Adds a mockserver handler that forwards dynamic_config to service's
    `dynamic-config-client` component.

    @ingroup userver_testsuite_fixtures
    """

    @mockserver.json_handler('/configs-service/configs/values')
    def _mock_configs(request):
        updates = dynamic_config_changelog.get_updated_since(
            dynamic_config.get_values_unsafe(),
            request.json.get('updated_since', ''),
            request.json.get('ids'),
        )
        response = {'configs': updates.values, 'updated_at': updates.timestamp}
        if updates.removed:
            response['removed'] = updates.removed
        return response

    @mockserver.json_handler('/configs-service/configs/status')
    def _mock_configs_status(_request):
        return {
            'updated_at': dynamic_config_changelog.timestamp.strftime(
                '%Y-%m-%dT%H:%M:%SZ',
            ),
        }


@pytest.fixture
def _userver_dynconfig_cache_control(dynamic_config_changelog: _Changelog):
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
        request, _dynamic_config_defaults_storage,
) -> typing.Callable[[], None]:
    def check():
        config_defaults = _dynamic_config_defaults_storage.snapshot
        assert config_defaults is not None

        if request.node.get_closest_marker('disable_config_check'):
            return

        unknown_configs = [
            key
            for marker in request.node.iter_markers('config')
            for key in marker.kwargs
            if key not in config_defaults
        ]

        if unknown_configs:
            message = _CHECK_CONFIG_ERROR.format(
                ', '.join(f'{key}=...' for key in sorted(unknown_configs)),
            )
            raise UnknownConfigError(message)

    return check
