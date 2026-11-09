import pytest
from pytest_userver import fixture_markers

from fixture_markers.visibility_marks import visibility
from fixture_markers.visibility_marks import VisibilityMark


@pytest.fixture(scope='session')
@visibility('unrelated-module')
def unrelated_module_visibility():
    return 'unrelated-module'


# Fixtures from scopes that do not apply to this test are omitted.
def test_unrelated_module_sees_own_fixture(request):
    infos = fixture_markers.get_infos(request, VisibilityMark)
    assert infos['unrelated_module_visibility'].origin == 'unrelated-module'
    assert infos['plugin_visibility'].origin == 'plugin'
    assert infos['conftest_visibility'].origin == 'conftest'
    assert 'module_visibility' not in infos
    assert 'nested_conftest_visibility' not in infos
