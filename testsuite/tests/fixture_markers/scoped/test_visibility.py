import pytest
from pytest_userver import fixture_markers

from fixture_markers.visibility_marks import visibility
from fixture_markers.visibility_marks import VisibilityMark


@pytest.fixture(scope='session')
@visibility('module')
def module_visibility():
    return 'module'


@pytest.fixture
@visibility('function')
def function_visibility():
    return 'function'


@pytest.fixture(scope='session')
def session_visibility_infos(request):
    return fixture_markers.get_infos(request, VisibilityMark)


# Collects fixtures from plugin, non-initial conftest, and this module.
def test_collects_plugin_conftest_module_and_function(request):
    infos = fixture_markers.get_infos(request, VisibilityMark)
    assert infos['plugin_visibility'].origin == 'plugin'
    assert infos['conftest_visibility'].origin == 'conftest'
    assert infos['module_visibility'].origin == 'module'
    assert infos['function_visibility'].origin == 'function'
    assert 'unrelated_module_visibility' not in infos
    assert 'nested_conftest_visibility' not in infos
    assert 'class_visibility' not in infos
    assert 'other_class_visibility' not in infos


# Fixtures narrower than request.scope are omitted.
def test_session_request_skips_narrower_scope(session_visibility_infos):
    infos = session_visibility_infos
    assert infos['plugin_visibility'].origin == 'plugin'
    assert infos['conftest_visibility'].origin == 'conftest'
    assert infos['module_visibility'].origin == 'module'
    assert 'function_visibility' not in infos
    assert 'class_visibility' not in infos


class TestClassVisibility:
    @pytest.fixture
    @visibility('class')
    def class_visibility(self):
        return 'class'

    # Collects a fixture defined on the test class.
    def test_class_fixture_is_visible(self, request):
        infos = fixture_markers.get_infos(request, VisibilityMark)
        assert infos['class_visibility'].origin == 'class'
        assert infos['plugin_visibility'].origin == 'plugin'
        assert infos['conftest_visibility'].origin == 'conftest'
        assert infos['module_visibility'].origin == 'module'
        assert 'other_class_visibility' not in infos
        assert 'unrelated_module_visibility' not in infos


class TestOtherClass:
    @pytest.fixture
    @visibility('other-class')
    def other_class_visibility(self):
        return 'other-class'

    # Fixtures from scopes that do not apply to this test are omitted.
    def test_does_not_see_sibling_class_fixture(self, request):
        infos = fixture_markers.get_infos(request, VisibilityMark)
        assert infos['other_class_visibility'].origin == 'other-class'
        assert 'class_visibility' not in infos
