import dataclasses

import pytest
from pytest_userver import fixture_markers

from fixture_markers.visibility_marks import VisibilityMark


@dataclasses.dataclass(frozen=True)
class _Seed:
    order: int


@dataclasses.dataclass(frozen=True)
class _Patch:
    pass


def _seed(order: int):
    def decorator(func):
        return fixture_markers.mark(func, _Seed(order=order))

    return decorator


def _patch(func):
    return fixture_markers.mark(func, _Patch())


def test_mark_sets_attr():
    def raw():
        return 'ok'

    seed = _Seed(order=9)
    fixture_markers.mark(raw, seed)
    marks = getattr(raw, fixture_markers._MARKS_ATTR)
    assert marks[_Seed] is seed

    patch = _Patch()
    fixture_markers.mark(raw, patch)
    assert marks[_Patch] is patch
    assert marks[_Seed] is seed


def test_mark_rejects_duplicate_type():
    def raw():
        return 'ok'

    seed = _Seed(order=9)
    fixture_markers.mark(raw, seed)
    with pytest.raises(ValueError, match='already has a _Seed mark'):
        fixture_markers.mark(raw, seed)
    with pytest.raises(ValueError, match='already has a _Seed mark'):
        fixture_markers.mark(raw, _Seed(order=1))


def test_mark_rejects_decorator_above_fixture():
    with pytest.raises(ValueError, match='already wrapped by @pytest.fixture'):

        @_patch
        @pytest.fixture
        def _wrong_order():
            return 'no'


# Fixtures from scopes that do not apply to this test are omitted.
def test_plugin_fixture_is_visible_outside_subtree(request):
    infos = fixture_markers.get_infos(request, VisibilityMark)
    assert infos['plugin_visibility'].origin == 'plugin'
    assert 'conftest_visibility' not in infos
    assert 'module_visibility' not in infos
    assert 'nested_conftest_visibility' not in infos


def test_lookup_key_is_resolved_once_per_call(request, monkeypatch):
    manager = request.session._fixturemanager
    original_lookup = manager.getfixturedefs
    original_signature = fixture_markers.inspect.signature
    expected = fixture_markers.get_infos(request, VisibilityMark)
    signature_calls = []

    def count_signature(func, *args, **kwargs):
        if func == manager.getfixturedefs:
            signature_calls.append(func)
        return original_signature(func, *args, **kwargs)

    def node_lookup(argname, node):
        assert node is request._pyfuncitem
        return original_lookup(argname, node)

    def nodeid_lookup(argname, nodeid):
        assert nodeid == request._pyfuncitem.nodeid
        return original_lookup(argname, request._pyfuncitem)

    monkeypatch.setattr(fixture_markers.inspect, 'signature', count_signature)
    for lookup in (node_lookup, nodeid_lookup):
        monkeypatch.setattr(manager, 'getfixturedefs', lookup)
        signature_calls.clear()
        assert list(fixture_markers.get_infos(request, VisibilityMark).items()) == list(expected.items())
        assert len(signature_calls) == 1
