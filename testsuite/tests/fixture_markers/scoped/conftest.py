import pytest

from fixture_markers.visibility_marks import visibility


@pytest.fixture(scope='session')
@visibility('conftest')
def conftest_visibility():
    return 'conftest'
