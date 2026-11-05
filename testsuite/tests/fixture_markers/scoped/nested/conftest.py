import pytest

from fixture_markers.visibility_marks import visibility


@pytest.fixture(scope='session')
@visibility('nested-conftest')
def nested_conftest_visibility():
    return 'nested-conftest'
