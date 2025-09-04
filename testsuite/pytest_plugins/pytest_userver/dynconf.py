"""
Python module that provides classes and constants
that may be useful when working with the
@ref dynamic_config_testsuite "dynamic config in testsuite".

@ingroup userver_testsuite
"""

import contextlib
import copy
import dataclasses
import datetime
from typing import Any
from typing import Dict
from typing import Iterable
from typing import Iterator
from typing import List
from typing import Optional
from typing import Set
from typing import Tuple

from pytest_userver.plugins import caches


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
    """Invalid dynamic config name in `@pytest.mark.config`"""


ConfigValuesDict = Dict[str, Any]


class _RemoveKey:
    pass


_REMOVE_KEY = _RemoveKey()


class _Missing:
    pass


_MISSING = _Missing()


@dataclasses.dataclass(frozen=True)
class _ConfigEntry:
    value: Any
    static_default_preferred: bool


_ConfigDict = Dict[str, _ConfigEntry | _RemoveKey]


def _create_config_dict(values: ConfigValuesDict, kill_switches_disabled: Optional[Set[str]] = None) -> _ConfigDict:
    if kill_switches_disabled is None:
        kill_switches_disabled = set()

    result = {}
    for key, value in values.items():
        static_default_preferred = key in kill_switches_disabled
        result[key] = _ConfigEntry(value, static_default_preferred)
    return result


@dataclasses.dataclass
class _ChangelogEntry:
    timestamp: str
    dirty_keys: Set[str]
    state: _ConfigDict
    prev_state: _ConfigDict

    @classmethod
    def new(
        cls,
        *,
        previous: Optional['_ChangelogEntry'],
        timestamp: str,
    ) -> '_ChangelogEntry':
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

    def update(self, values: _ConfigDict):
        for key, value in values.items():
            if value == self.prev_state.get(key, _MISSING):
                self.dirty_keys.discard(key)
            else:
                self.dirty_keys.add(key)
        self.state.update(values)


@dataclasses.dataclass(frozen=True)
class _Updates:
    timestamp: str
    values: ConfigValuesDict
    removed: List[str]
    kill_switches_disabled: List[str]

    def is_empty(self) -> bool:
        return not self.values and not self.removed


class _Changelog:
    timestamp: datetime.datetime
    committed_entries: List[_ChangelogEntry]
    staged_entry: _ChangelogEntry

    def __init__(self):
        self.timestamp = datetime.datetime.fromtimestamp(
            0,
            datetime.timezone.utc,
        )
        self.committed_entries = []
        self.staged_entry = _ChangelogEntry.new(
            timestamp=self.service_timestamp(),
            previous=None,
        )

    def service_timestamp(self) -> str:
        return self.timestamp.strftime('%Y-%m-%dT%H:%M:%SZ')

    def next_timestamp(self) -> str:
        self.timestamp += datetime.timedelta(seconds=1)
        return self.service_timestamp()

    def commit(self) -> _ChangelogEntry:
        """Commit staged changed if any and return last committed entry."""
        entry = self.staged_entry
        if entry.has_changes or not self.committed_entries:
            self.staged_entry = _ChangelogEntry.new(
                timestamp=self.next_timestamp(),
                previous=entry,
            )
            self.committed_entries.append(entry)
        return self.committed_entries[-1]

    def get_updated_since(
        self,
        config_dict: _ConfigDict,
        updated_since: str,
        ids: Optional[List[str]] = None,
    ) -> _Updates:
        entry = self.commit()
        config_dict, removed = self._get_updated_since(config_dict, updated_since)
        if ids:
            config_dict = {name: config_dict[name] for name in ids if name in config_dict}
            removed = [name for name in removed if name in ids]

        values = {}
        kill_switches_disabled = []
        for name, config_entry in config_dict.items():
            values[name] = config_entry.value
            if config_entry.static_default_preferred:
                kill_switches_disabled.append(name)

        return _Updates(
            timestamp=entry.timestamp,
            values=values,
            removed=removed,
            kill_switches_disabled=kill_switches_disabled,
        )

    def _get_updated_since(
        self,
        config_dict: _ConfigDict,
        updated_since: str,
    ) -> Tuple[_ConfigDict, List[str]]:
        if not updated_since:
            return config_dict, []
        dirty_keys = set()
        last_known_state = {}
        for entry in reversed(self.committed_entries):
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
            config_entry = config_dict.get(key, _REMOVE_KEY)
            if last_known_state.get(key, _MISSING) != config_entry:
                if config_entry is _REMOVE_KEY:
                    removed.append(key)
                else:
                    result[key] = config_entry
        return result, removed

    def add_entries(self, config_dict: _ConfigDict) -> None:
        self.staged_entry.update(config_dict)

    @contextlib.contextmanager
    def rollback(self, defaults: ConfigValuesDict) -> Iterator[None]:
        try:
            yield
        finally:
            self._do_rollback(defaults)

    def _do_rollback(self, defaults: ConfigValuesDict) -> None:
        if not self.committed_entries:
            return

        maybe_dirty = set()
        for entry in self.committed_entries:
            maybe_dirty.update(entry.dirty_keys)

        last = self.committed_entries[-1]
        last_state = last.state
        config_dict = _create_config_dict(defaults)
        dirty_keys = set()
        reverted = {}
        for key in maybe_dirty:
            original = config_dict.get(key, _REMOVE_KEY)
            if last_state[key] != original:
                dirty_keys.add(key)
            reverted[key] = original

        entry = _ChangelogEntry(
            timestamp=last.timestamp,
            state=last.state,
            dirty_keys=dirty_keys,
            prev_state={},
        )
        self.committed_entries = [entry]
        self.staged_entry = _ChangelogEntry(
            timestamp=self.staged_entry.timestamp,
            dirty_keys=dirty_keys.copy(),
            state=reverted,
            prev_state=entry.state,
        )


class DynamicConfig:
    """
    @brief Simple dynamic config backend.

    @see @ref pytest_userver.plugins.dynamic_config.dynamic_config "dynamic_config"
    """

    def __init__(
        self,
        *,
        initial_values: ConfigValuesDict,
        defaults: Optional[ConfigValuesDict],
        config_cache_components: Iterable[str],
        cache_invalidation_state: caches.InvalidationState,
        changelog: _Changelog,
    ) -> None:
        self._values = initial_values.copy()
        self._kill_switches_disabled = set()
        # Defaults are only there for convenience, to allow accessing them
        # in tests using dynamic_config.get. They are not sent to the service.
        self._defaults = defaults
        self._cache_invalidation_state = cache_invalidation_state
        self._config_cache_components = config_cache_components
        self._changelog = changelog

    def set_values(self, values: ConfigValuesDict) -> None:
        self.set_values_unsafe(copy.deepcopy(values))

    def set_values_unsafe(self, values: ConfigValuesDict) -> None:
        self._values.update(values)
        for key in values:
            self._kill_switches_disabled.discard(key)

        config_dict = _create_config_dict(values)
        self._changelog.add_entries(config_dict)
        self._sync_with_service()

    def set(self, **values) -> None:
        self.set_values(values)

    def switch_to_static_default(self, *keys: str) -> None:
        for key in keys:
            self._kill_switches_disabled.add(key)

        config_dict = _create_config_dict(
            values={key: self._values.get(key, None) for key in keys}, kill_switches_disabled=set(keys)
        )
        self._changelog.add_entries(config_dict)
        self._sync_with_service()

    def switch_to_dynamic_value(self, *keys: str) -> None:
        for key in keys:
            self._kill_switches_disabled.discard(key)

        config_dict = _create_config_dict(values={key: self._values[key] for key in keys if key in self._values})
        self._changelog.add_entries(config_dict)
        self._sync_with_service()

    def get_values_unsafe(self) -> ConfigValuesDict:
        return self._values

    def get_kill_switches_disabled_unsafe(self) -> Set[str]:
        return self._kill_switches_disabled

    def get(self, key: str, default: Any = None) -> Any:
        if key in self._values and key not in self._kill_switches_disabled:
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

    def remove_values(self, keys: Iterable[str]) -> None:
        extra_keys = set(keys).difference(self._values.keys())
        if extra_keys:
            raise DynamicConfigNotFoundError(
                f'Attempting to remove nonexistent configs: {extra_keys}',
            )
        for key in keys:
            self._values.pop(key)
            self._kill_switches_disabled.discard(key)

        self._changelog.add_entries({key: _REMOVE_KEY for key in keys})
        self._sync_with_service()

    def remove(self, key: str) -> None:
        return self.remove_values([key])

    @contextlib.contextmanager
    def modify(self, key: str) -> Any:
        value = self.get(key)
        yield value
        self.set_values({key: value})

    @contextlib.contextmanager
    def modify_many(
        self,
        *keys: Tuple[str, ...],
    ) -> Tuple[Any, ...]:
        values = tuple(self.get(key) for key in keys)
        yield values
        self.set_values(dict(zip(keys, values)))

    def _sync_with_service(self) -> None:
        self._cache_invalidation_state.invalidate(
            self._config_cache_components,
        )


class UseStaticDefault:
    pass


USE_STATIC_DEFAULT = UseStaticDefault()
