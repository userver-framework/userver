import pytest


@pytest.mark.includetest
def test_sql_coverage(sql_coverage, on_uncovered):
    sql_coverage.validate(on_uncovered)
