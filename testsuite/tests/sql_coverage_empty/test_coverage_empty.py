import pytest


class CustomError(Exception):
    pass


def test_sql_coverage(sql_coverage):
    def _on_uncovered(uncovered_statements):
        sorted_statements = sorted(list(uncovered_statements))
        raise CustomError(f'Uncovered SQL statements: {sorted_statements}')

    with pytest.raises(CustomError) as exc:
        sql_coverage.validate(_on_uncovered)

    assert str(exc.value) == "Uncovered SQL statements: ['query1', 'query2', 'query3']"
