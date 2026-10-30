"""Exercise pytest API generations even when Arcadia bundles an older pytest."""

from types import SimpleNamespace

import pytest
from pytest_userver import fixture_markers

from fixture_markers.guarantees import OrderInfo

_SCOPES = ['function', 'class', 'module', 'package', 'session']


def _definition(name, scope, value):
    def fixture():
        pytest.fail('Marker discovery must not execute fixtures')

    fixture_markers.mark(fixture, OrderInfo(value))
    return SimpleNamespace(argname=name, scope=scope, func=fixture, baseid='', has_location=False, node=None)


class _NodeIdManager:
    def __init__(self, definitions, item):
        self._arg2fixturedefs = definitions
        self.item = item

    def getfixturedefs(self, name, nodeid):
        assert nodeid == self.item.nodeid
        return self._arg2fixturedefs[name]


class _NodeManager(_NodeIdManager):
    def getfixturedefs(self, name, node):
        assert node is self.item
        return self._arg2fixturedefs[name]


class _SplitRegistryManager:
    """No _arg2fixturedefs attribute, just like pytest 9.2's FixtureManager."""

    def __init__(self, definitions, item):
        self.definitions = definitions
        self.item = item
        self.enumerations = 0
        self.lookups = []

    def _get_all_fixture_defs(self):
        self.enumerations += 1
        for definitions in self.definitions.values():
            yield from definitions or ()

    def getfixturedefs(self, name, node):
        assert node is self.item
        self.lookups.append(name)
        return self.definitions[name]


@pytest.mark.parametrize('manager_type', [_NodeIdManager, _NodeManager, _SplitRegistryManager])
@pytest.mark.parametrize('request_scope', _SCOPES)
def test_discovery_across_pytest_apis_and_scopes(manager_type, request_scope):
    definitions = {scope: [_definition(scope, scope, scope)] for scope in _SCOPES}
    item = SimpleNamespace(nodeid='tests/test_discovery.py::test')
    manager = manager_type(definitions, item)
    request = SimpleNamespace(scope=request_scope, session=SimpleNamespace(_fixturemanager=manager), _pyfuncitem=item)

    # Avoid using fixture_markers' private rank table to compute the expectation.
    expected_scopes = _SCOPES[_SCOPES.index(request_scope) :]
    assert fixture_markers.get_infos(request, OrderInfo) == {scope: OrderInfo(scope) for scope in expected_scopes}


def test_new_registry_deduplicates_names_and_uses_lookup_for_visibility():
    item = SimpleNamespace(nodeid='tests/test_discovery.py::test')
    original = _definition('overridden', 'session', 'original')
    winner = _definition('overridden', 'session', 'winner')
    legacy = _definition('legacy_registration', 'session', 'legacy')
    hidden = _definition('hidden', 'session', 'hidden')
    manager = _SplitRegistryManager(
        {'overridden': [original, winner], 'legacy_registration': [legacy], 'hidden': [hidden]}, item
    )

    # The enumeration includes globally known definitions, while getfixturedefs
    # must still decide visibility for the requesting node.
    def getfixturedefs(name, node):
        assert node is item
        manager.lookups.append(name)
        return () if name == 'hidden' else manager.definitions[name]

    manager.getfixturedefs = getfixturedefs
    request = SimpleNamespace(scope='session', session=SimpleNamespace(_fixturemanager=manager), _pyfuncitem=item)
    assert list(fixture_markers._iter_visible_fixtures(request)) == [winner, legacy]
    assert manager.enumerations == 1
    assert manager.lookups == ['overridden', 'legacy_registration', 'hidden']


def test_empty_new_registry_does_not_fall_back_to_removed_attribute():
    item = SimpleNamespace(nodeid='tests/test_discovery.py::test')
    manager = _SplitRegistryManager({}, item)
    request = SimpleNamespace(scope='function', session=SimpleNamespace(_fixturemanager=manager), _pyfuncitem=item)
    assert fixture_markers.get_infos(request, OrderInfo) == {}
    assert manager.enumerations == 1


@pytest.mark.parametrize('missing', [None, ()])
def test_old_registry_skips_missing_or_invisible_fixture(missing):
    item = SimpleNamespace(nodeid='tests/test_discovery.py::test')
    manager = _NodeManager({'hidden': missing}, item)
    request = SimpleNamespace(scope='function', session=SimpleNamespace(_fixturemanager=manager), _pyfuncitem=item)
    assert fixture_markers.get_infos(request, OrderInfo) == {}
