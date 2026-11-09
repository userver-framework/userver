"""
Utilities for attaching typed marks to pytest fixture functions
and querying those marks during a test session.

Use @ref pytest_userver.fixture_markers.mark to store an info object on the original function.
Use @ref pytest_userver.fixture_markers.get_infos to retrieve fixtures tagged with a particular
info type among those visible to a request.

Marks are independent of `@pytest.fixture`: apply the mark to the
original function, then wrap with `@pytest.fixture`.

@ingroup userver_testsuite
@see @ref scripts/docs/en/userver/fixture_markers.md
"""

from __future__ import annotations

from collections.abc import Callable
from collections.abc import Iterator
from collections.abc import Sequence
from dataclasses import dataclass
import functools
import inspect
import itertools
from typing import Any
from typing import cast
from typing import Generic
from typing import TypeVar

import pytest

try:
    from _pytest.fixtures import is_visibility_more_specific
except ImportError:  # pragma: no cover
    is_visibility_more_specific = None

__all__ = [
    'get_infos',
    'mark',
]

I = TypeVar('I')  # noqa: E741

_MARKS_ATTR = '_testsuite_fixture_marks'
_MARKS_ORDER_ATTR = '_testsuite_fixture_marks_order'
_mark_order_counter = itertools.count()
_SCOPE_RANK = {
    'function': 0,
    'class': 1,
    'module': 2,
    'package': 3,
    'session': 4,
}


@dataclass(frozen=True)
class _MarkedFixture(Generic[I]):
    origin: pytest.FixtureDef[object]
    stamp: int
    name: str
    info: I


def mark(
    func: Callable[..., object],
    info: object,
    /,
) -> Callable[..., object]:
    """
    Attach *info* to the original function *func*.

    Creates the marks dict if it is missing, otherwise updates it.
    The *info* type is the lookup key for @ref pytest_userver.fixture_markers.get_infos. Each type
    may be attached at most once.

    *func* must be the original function, not a `@pytest.fixture`
    wrapper. Apply the mark under `@pytest.fixture`.

    @see @ref scripts/docs/en/userver/fixture_markers.md

    @param func The original fixture function.
    @param info Metadata instance to store.
    @returns *func*, unchanged.
    """
    if _is_pytest_fixture_wrapper(func):
        name = getattr(func, '__qualname__', func.__name__)
        raise ValueError(
            f'{name!r} is already wrapped by @pytest.fixture; apply the mark under @pytest.fixture, not above it',
        )

    marks = getattr(func, _MARKS_ATTR, None)
    if marks is None:
        marks = {}
        setattr(func, _MARKS_ATTR, marks)
    info_type = type(info)
    if (existing := marks.get(info_type)) is not None:
        name = getattr(func, '__qualname__', func.__name__)
        raise ValueError(
            f'{name!r} already has a {info_type.__name__} mark ({existing!r}); cannot attach another ({info!r})',
        )
    marks[info_type] = info
    # Decorations run in definition order within a module; pytest later
    # registers fixtures via dir(), which is alphabetical — stamp order here.
    if getattr(func, _MARKS_ORDER_ATTR, None) is None:
        setattr(func, _MARKS_ORDER_ATTR, next(_mark_order_counter))
    return func


def get_infos(
    request: pytest.FixtureRequest,
    info_type: type[I],
    /,
) -> dict[str, I]:
    """
    Return marked fixtures of *info_type* that are visible to a request.

    Walks fixture definitions known to pytest, keeps those applicable to
    the requesting test, and skips fixtures whose scope is narrower than
    `request.scope`. Metadata is matched by its exact type, not subclasses.
    Discovery does not execute fixtures or require them to be test arguments.

    The winning fixture definition is the one pytest would call. If it
    has no mark of *info_type*, the mark is inherited from the nearest
    overridden definition that has one. A mark on the winner replaces
    the inherited mark. There is no way to drop an inherited mark.

    Fixtures within a single module are ordered by definition order. An
    override takes the position of the original fixture in that order — it
    is inserted in place of the fixture it overrides.

    Fixtures from different modules follow the usual pytest definition-site
    priorities: non-conftest plugins (registration order), then conftest
    modules from outer to inner (less specific first, same as pytest
    override chains), then test modules, then test classes.

    @see @ref scripts/docs/en/userver/fixture_markers.md

    @param request The pytest fixture request object.
    @param info_type The info class whose tagged fixtures you want
        to look up.
    @returns `dict[str, I]` mapping each tagged fixture's name to the
        *info* object that was passed to @ref pytest_userver.fixture_markers.mark.
        The dict is a fresh copy; mutating it has no effect on stored
        data.
    """
    collected: list[_MarkedFixture[I]] = []
    for fixturedef in _iter_visible_fixtures(request):
        marked = _marked_fixture_for_name(request, fixturedef.argname, info_type)
        if marked is not None:
            collected.append(marked)
    plugin_key_order: dict[str, int] = {}
    for i, item in enumerate(collected):
        pk = _plugin_key(item.origin.func)
        if pk not in plugin_key_order:
            plugin_key_order[pk] = i
    collected.sort(
        key=functools.cmp_to_key(
            lambda lhs, rhs: _cmp_marked_fixtures(lhs, rhs, plugin_key_order),
        ),
    )
    return {item.name: item.info for item in collected}


def _is_pytest_fixture_wrapper(func: object) -> bool:
    if getattr(func, '_pytestfixturefunction', None):
        return True
    return type(func).__name__ == 'FixtureFunctionDefinition'


def _get_fixturedefs(
    fixture_manager: Any,
    name: str,
    request: pytest.FixtureRequest,
) -> Sequence[pytest.FixtureDef[object]] | None:
    item = request._pyfuncitem
    params = inspect.signature(fixture_manager.getfixturedefs).parameters
    key = item if list(params)[1] == 'node' else item.nodeid
    return fixture_manager.getfixturedefs(name, key)


def _iter_visible_fixtures(
    request: pytest.FixtureRequest,
) -> Iterator[pytest.FixtureDef[object]]:
    fixture_manager = request.session._fixturemanager
    invoking_rank = _SCOPE_RANK[request.scope]

    # pytest 9.2 splits node-based and legacy nodeid-based registrations.
    get_all_fixture_defs = getattr(fixture_manager, '_get_all_fixture_defs', None)
    if get_all_fixture_defs is not None:
        fixture_names = dict.fromkeys(fixturedef.argname for fixturedef in get_all_fixture_defs())
    else:
        fixture_names = fixture_manager._arg2fixturedefs

    for name in fixture_names:
        matched = _get_fixturedefs(fixture_manager, name, request)
        if not matched:
            continue
        winning = matched[-1]
        if _SCOPE_RANK[winning.scope] < invoking_rank:
            continue
        yield winning


def _marked_fixture_for_name(
    request: pytest.FixtureRequest,
    name: str,
    info_type: type[I],
) -> _MarkedFixture[I] | None:
    fixture_manager = request.session._fixturemanager
    matched = _get_fixturedefs(fixture_manager, name, request)
    if not matched:
        return None
    info = _inherited_info(matched, info_type)
    if info is None:
        return None
    origin = next(fixturedef for fixturedef in matched if _mark_info(fixturedef, info_type) is not None)
    return _MarkedFixture(
        origin=origin,
        stamp=getattr(origin.func, _MARKS_ORDER_ATTR),
        name=name,
        info=info,
    )


def _inherited_info(
    matched: Sequence[pytest.FixtureDef[object]],
    info_type: type[I],
) -> I | None:
    # matched is ordered from the least specific definition to the winner.
    for fixturedef in reversed(matched):
        info = _mark_info(fixturedef, info_type)
        if info is not None:
            return info
    return None


def _cmp_from_lt(a: Any, b: Any) -> int:
    return (a > b) - (a < b)


def _cmp_marked_fixtures(
    left: _MarkedFixture[Any],
    right: _MarkedFixture[Any],
    plugin_key_order: dict[str, int],
) -> int:
    # Less specific definition site first.
    site = _cmp_sites(left.origin, right.origin)
    if site:
        return site
    # Same site: order by plugin registration, then by decoration stamp.
    left_key = _plugin_key(left.origin.func)
    right_key = _plugin_key(right.origin.func)
    if left_key != right_key:
        return _cmp_from_lt(plugin_key_order[left_key], plugin_key_order[right_key])
    return _cmp_from_lt(left.stamp, right.stamp)


def _plugin_key(func: Any) -> str:
    module: str = getattr(func, '__module__', '') or ''
    qualname: str = getattr(func, '__qualname__', '') or ''
    class_part = qualname.rsplit('.', 1)[0] if '.' in qualname else ''
    return f'{module}.{class_part}' if class_part else module


def _cmp_sites(
    left: pytest.FixtureDef[object],
    right: pytest.FixtureDef[object],
) -> int:
    more_specific = is_visibility_more_specific or _legacy_more_specific
    if more_specific(left, right):
        return 1
    if more_specific(right, left):
        return -1
    return 0


def _legacy_more_specific(
    candidate: pytest.FixtureDef[object],
    other: pytest.FixtureDef[object],
) -> bool:
    # pytest without is_visibility_more_specific: plugins, then outer → inner.
    if _has_location(candidate) != _has_location(other):
        return _has_location(candidate)
    inner, outer = candidate.baseid, other.baseid
    if _is_nested_baseid(outer, inner):
        return True
    if inner == outer or _is_nested_baseid(inner, outer):
        return False
    raise AssertionError(
        f'expected nested fixture baseids, got {inner!r} and {outer!r}',
    )


def _has_location(fixturedef: pytest.FixtureDef[object]) -> bool:
    return bool(getattr(fixturedef, '_has_location', fixturedef.has_location))


def _is_nested_baseid(prefix: str, full: str) -> bool:
    return prefix != full and (prefix == '' or (full.startswith(prefix) and full[len(prefix)] in '/:'))


def _mark_info(
    fixturedef: pytest.FixtureDef[object],
    info_type: type[I],
) -> I | None:
    marks = getattr(fixturedef.func, _MARKS_ATTR, None)
    if not marks:
        return None
    info = marks.get(info_type)
    if type(info) is info_type:
        return cast(I, info)
    return None
