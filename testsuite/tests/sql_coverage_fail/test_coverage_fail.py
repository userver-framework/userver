from pytest_userver.test_sql_coverage import *  # noqa
import pytest


@pytest.fixture
def on_uncovered(counter_increment):
    def _on_uncovered(uncovered_statements):
        counter_increment()

    return _on_uncovered
