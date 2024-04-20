"""
Fixtures for controlling userver caches.
"""
import copy
import enum
import types
import typing

import pytest

from testsuite.daemons.pytest_plugin import DaemonInstance


class UserverCachePlugin:
    def __init__(self):
        self._hooks = {}

    @property
    def userver_cache_control_hooks(self) -> typing.Dict[str, str]:
        return self._hooks

    def pytest_plugin_registered(self, plugin, manager):
        if not isinstance(plugin, types.ModuleType):
            return
        uhooks = getattr(plugin, 'USERVER_CACHE_CONTROL_HOOKS', None)
        if uhooks is None:
            return
        if not isinstance(uhooks, dict):
            raise RuntimeError(
                f'USERVER_CACHE_CONTROL_HOOKS must be dictionary: '
                f'{{cache_name: fixture_name}}, got {uhooks} instead',
            )
        for cache_name, fixture_name in uhooks.items():
            if cache_name in self._hooks:
                raise RuntimeError(
                    f'USERVER_CACHE_CONTROL_HOOKS: hook already registered '
                    f'for cache {cache_name}',
                )
            self._hooks[cache_name] = fixture_name


class InvalidationState:
    def __init__(self):
        # None means that we should update all caches.
        # We invalidate all caches at the start of each test.
        self._invalidated_caches: typing.Optional[typing.Set[str]] = None

    def invalidate_all(self) -> None:
        self._invalidated_caches = None

    def invalidate(self, caches: typing.Iterable[str]) -> None:
        if self._invalidated_caches is not None:
            self._invalidated_caches.update(caches)

    @property
    def should_update_all_caches(self) -> bool:
        return self._invalidated_caches is None

    @property
    def caches_to_update(self) -> typing.FrozenSet[str]:
        assert self._invalidated_caches is not None
        return frozenset(self._invalidated_caches)

    @property
    def has_caches_to_update(self) -> bool:
        caches = self._invalidated_caches
        return caches is None or bool(caches)

    def on_caches_updated(self, caches: typing.Iterable[str]) -> None:
        if self._invalidated_caches is not None:
            self._invalidated_caches.difference_update(caches)

    def on_all_caches_updated(self) -> None:
        self._invalidated_caches = set()

    def assign_copy(self, other: 'InvalidationState') -> None:
        # pylint: disable=protected-access
        self._invalidated_caches = copy.deepcopy(other._invalidated_caches)


class CacheControlAction(enum.Enum):
    FULL = 0
    INCREMENTAL = 1
    EXCLUDE = 2


class CacheControlRequest:
    action = CacheControlAction.FULL

    def exclude(self) -> None:
        """Exclude cache from update."""
        self.action = CacheControlAction.EXCLUDE

    def incremental(self) -> None:
        """Request incremental update instead of full."""
        self.action = CacheControlAction.INCREMENTAL


class CacheControl:
    def __init__(
            self,
            *,
            enabled: bool,
            context: typing.Dict,
            fixtures: typing.List[str],
            caches_disabled: typing.Set[str],
    ):
        self._enabled = enabled
        self._context = context
        self._fixtures = fixtures
        self._caches_disabled = caches_disabled

    def query_caches(
            self, cache_names: typing.Optional[typing.List[str]],
    ) -> typing.Tuple[
        typing.Dict, typing.List[typing.Tuple[str, CacheControlAction]],
    ]:
        """Query cache control handlers.

        Returns pair (staged, [(cache_name, action), ...])
        """
        if not self._enabled:
            if cache_names is None:
                cache_names = self._fixtures.keys()
            return {cache_name: None for cache_name in cache_names}, []
        staged = {}
        actions = []
        for cache_name, fixture in self._fixtures.items():
            if cache_names and cache_name not in cache_names:
                continue
            if cache_name in self._caches_disabled:
                staged[cache_name] = None
                continue
            context = self._context.get(cache_name)
            request = CacheControlRequest()
            staged[cache_name] = fixture(request, context)
            actions.append((cache_name, request.action))
        return staged, actions

    def commit_staged(self, staged: typing.Dict[str, typing.Any]) -> None:
        """Apply recently commited state."""
        self._context.update(staged)


def pytest_configure(config):
    config.pluginmanager.register(UserverCachePlugin(), 'userver_cache')
    config.addinivalue_line(
        'markers', 'userver_cache_control_disabled: disable cache control',
    )


@pytest.fixture
def cache_invalidation_state() -> InvalidationState:
    """
    A fixture for notifying the service of changes in cache data sources.

    Intended to be used by other fixtures that represent those data sources,
    not by tests directly.

    @ingroup userver_testsuite_fixtures
    """
    return InvalidationState()


@pytest.fixture(scope='session')
def _userver_cache_control_context() -> typing.Dict:
    return {}


@pytest.fixture
def _userver_cache_fixtures(
        pytestconfig, request,
) -> typing.Dict[str, typing.Callable]:
    plugin: UserverCachePlugin = pytestconfig.pluginmanager.get_plugin(
        'userver_cache',
    )
    result = {}
    for cache_name, fixture_name in plugin.userver_cache_control_hooks.items():
        result[cache_name] = request.getfixturevalue(fixture_name)
    return result


@pytest.fixture
def userver_cache_control(
        _userver_cache_control_context, _userver_cache_fixtures, request,
) -> typing.Callable[[DaemonInstance], CacheControl]:
    """Userver cache control handler.

    To install per cache handler use USERVER_CACHE_CONTROL_HOOKS variable
    in your pytest plugin:

    @code
    USERVER_CACHE_CONTROL_HOOKS = {
        'my-cache-name': 'my_cache_cc',
    }

    @pytest.fixture
    def my_cache_cc(my_cache_context):
        def cache_control(request, state):
            new_state = my_cache_context.get_state()
            if state == new_state:
                # Cache is already up to date, no need to update
                request.exclude()
            else:
                # Request incremental update, if you cache supports it
                request.incremental()
            return new_state
        return cache_control
    @endcode

    @ingroup userver_testsuite_fixtures
    """
    enabled = True
    caches_disabled = set()

    def userver_cache_control_disabled(
            caches: typing.Sequence[str] = None, *, reason: str,
    ):
        if caches is not None:
            caches_disabled.update(caches)
            return enabled
        return False

    for mark in request.node.iter_markers('userver_cache_control_disabled'):
        enabled = userver_cache_control_disabled(*mark.args, **mark.kwargs)

    def get_cache_control(daemon: DaemonInstance):
        context = _userver_cache_control_context.setdefault(daemon.id, {})
        return CacheControl(
            context=context,
            fixtures=_userver_cache_fixtures,
            enabled=enabled,
            caches_disabled=caches_disabled,
        )

    return get_cache_control
