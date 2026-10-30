from dataclasses import dataclass

import pytest
from pytest_userver import config
from pytest_userver import fixture_markers
from pytest_userver import service

from fixture_markers.guarantees import ordered
from fixture_markers.guarantees import OrderInfo
from fixture_markers.guarantees import OtherInfo


@pytest.fixture(scope='session')
@ordered('module-first')
def z_module():
    return 'module-first'


@pytest.fixture(name='overridden', scope='session')
def _aliased_override():
    return 'winner'


@pytest.fixture(scope='session')
@ordered('module-last')
def a_module():
    return 'module-last'


@pytest.fixture
def narrowed():
    return 'narrow'


@pytest.fixture(scope='session')
def session_infos(request):
    return fixture_markers.get_infos(request, OrderInfo)


@pytest.fixture(scope='session')
def inherited_config_patch():
    def patch(config_yaml, config_vars):
        config_yaml['patched'] = True
        config_vars['patched'] = True

    return patch


@pytest.fixture
def inherited_service_dependency():
    return 'dependency override'


@dataclass(frozen=True)
class BaseInfo:
    pass


@dataclass(frozen=True)
class DerivedInfo(BaseInfo):
    pass


def _derived(func):
    return fixture_markers.mark(func, DerivedInfo())


@pytest.fixture
@_derived
def derived_metadata():
    pytest.fail('Metadata discovery must not instantiate the fixture')


# get_infos() order: plugins (registration order) → conftest → module → class.
# Within each plugin, stamp (decoration order) determines position.
# LaterPlugin is defined before GuaranteesPlugin (lower stamps) but registered
# after it, so it sorts after GuaranteesPlugin.
_EXPECTED_ORDER = [
    'z_first',  # GuaranteesPlugin
    'overridden',
    'a_last',
    'narrowed',
    'z_later_plugin',  # LaterPlugin (registered after GuaranteesPlugin)
    'a_later_plugin',
    'z_conftest',  # conftest
    'a_conftest',
    'z_module',  # test module
    'a_module',
]

_EXPECTED_ORDER_WITH_CLASS = [
    *_EXPECTED_ORDER,
    'z_class',  # TestMarkedClassOverride
    'a_class',
]


def test_metadata_type_matching_is_exact(request):
    assert fixture_markers.get_infos(request, BaseInfo) == {}
    assert fixture_markers.get_infos(request, DerivedInfo) == {'derived_metadata': DerivedInfo()}


def test_get_infos_order_matches_pytest_priority(request):
    """Regression: lower stamp must not override plugin registration order."""
    infos = fixture_markers.get_infos(request, OrderInfo)
    assert list(infos) == _EXPECTED_ORDER


def test_definition_order_and_original_override_position(request):
    infos = fixture_markers.get_infos(request, OrderInfo)
    assert infos['overridden'] == OrderInfo('nearest')
    assert '_aliased_override' not in infos
    # Querying only inspects definitions. Executing the original or intermediate
    # overridden fixture would fail, and resolving the name must use the winner.
    assert request.getfixturevalue('overridden') == 'winner'


def test_inheritance_is_per_metadata_type(request):
    assert fixture_markers.get_infos(request, OrderInfo)['overridden'] == OrderInfo('nearest')
    assert fixture_markers.get_infos(request, OtherInfo) == {'overridden': OtherInfo('other-type')}


def test_results_are_fresh_dictionaries(request):
    first = fixture_markers.get_infos(request, OrderInfo)
    second = fixture_markers.get_infos(request, OrderInfo)
    assert first is not second
    assert first == second
    first.clear()
    second['overridden'] = OrderInfo('modified-result')
    again = fixture_markers.get_infos(request, OrderInfo)
    # Each call returns a fresh independent result; earlier mutations don't affect it.
    assert again == fixture_markers.get_infos(request, OrderInfo)
    assert again['overridden'] == OrderInfo('nearest')


def test_unknown_metadata_type_returns_empty(request):
    @dataclass(frozen=True)
    class Unknown:
        pass

    assert fixture_markers.get_infos(request, Unknown) == {}


def test_scope_filter_uses_winning_definition(session_infos, request):
    # A session-scoped marked original must not leak into a session request
    # after a function-scoped unmarked override wins.
    assert 'narrowed' not in session_infos
    assert fixture_markers.get_infos(request, OrderInfo)['narrowed'] == OrderInfo('wide')


def test_public_config_decorator_is_inherited_without_redecoration(request):
    infos = fixture_markers.get_infos(request, config._ConfigPatchInfo)
    assert 'inherited_config_patch' in infos
    patch = request.getfixturevalue('inherited_config_patch')
    config_yaml, config_vars = {}, {}
    patch(config_yaml, config_vars)
    assert config_yaml == {'patched': True}
    assert config_vars == {'patched': True}


def test_public_dependency_decorator_is_inherited_without_redecoration(request):
    infos = fixture_markers.get_infos(request, service._ServiceDependencyInfo)
    assert 'inherited_service_dependency' in infos
    assert request.getfixturevalue('inherited_service_dependency') == 'dependency override'


@pytest.mark.parametrize('decorate', [config.patch, service.dependency])
def test_public_decorators_return_the_original_function(decorate):
    def raw():
        return 'unchanged'

    assert decorate(raw) is raw
    assert raw() == 'unchanged'


@pytest.mark.parametrize('decorate', [config.patch, service.dependency])
def test_public_decorators_reject_wrong_decorator_order(decorate):
    @pytest.fixture
    def wrapped():
        return 'value'

    with pytest.raises(ValueError, match='already wrapped by @pytest.fixture'):
        decorate(wrapped)


class TestUnmarkedClassOverride:
    @pytest.fixture(scope='session')
    def overridden(self):
        return 'class-winner'

    def test_inherits_through_multiple_unmarked_overrides(self, request):
        infos = fixture_markers.get_infos(request, OrderInfo)
        assert infos['overridden'] == OrderInfo('nearest')
        assert request.getfixturevalue('overridden') == 'class-winner'


class TestMarkedClassOverride:
    @pytest.fixture(scope='session')
    @ordered('class-first')
    def z_class(self):
        return 'class-first'

    @pytest.fixture(scope='session')
    @ordered('replacement')
    def overridden(self):
        return 'marked-winner'

    @pytest.fixture(scope='session')
    @ordered('class-last')
    def a_class(self):
        return 'class-last'

    def test_replacement_keeps_original_position(self, request):
        infos = fixture_markers.get_infos(request, OrderInfo)
        assert list(infos) == _EXPECTED_ORDER_WITH_CLASS
        assert infos['overridden'] == OrderInfo('replacement')
        assert fixture_markers.get_infos(request, OtherInfo)['overridden'] == OtherInfo('other-type')
        assert request.getfixturevalue('overridden') == 'marked-winner'
