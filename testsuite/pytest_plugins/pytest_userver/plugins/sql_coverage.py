"""
Plugin that imports the required fixtures for checking SQL/YQL coverage. See
@ref sql_coverage_test_info "SQL coverage tests" for more info.

@ingroup userver_testsuite_fixtures
"""

from typing import Set

import pytest

from . import coverage

_SQL_COVERAGE_TEST_NAME = 'test_sql_coverage'


@pytest.fixture
def on_uncovered():
    """
    Called when the coverage is incomplete.

    Override this fixture to change the way uncovered statements are reported or to ignore some of the statements
    from coverage report.

    See @ref sql_coverage_test_info "SQL coverage tests" for more info.

    @ingroup userver_testsuite_fixtures
    """

    def _on_uncovered(uncovered_statements: Set[str]):
        msg = f'Uncovered SQL/YQL statements: {uncovered_statements}'
        raise coverage.UncoveredError(msg)

    return _on_uncovered


class Coverage:
    """
    Contains data about the current coverage of statements.
    """

    def __init__(self, files: Set[str]):
        self.covered_statements: Set[str] = set()
        self.uncovered_statements: Set[str] = files
        self.extra_covered_statements: Set[str] = set()

    def cover(self, statement: str) -> None:
        if statement in self.uncovered_statements:
            self.covered_statements.add(statement)
            self.uncovered_statements.remove(statement)
        elif statement not in self.covered_statements:
            self.extra_covered_statements.add(statement)

    def validate(self, uncovered_callback: callable) -> None:
        if self.uncovered_statements:
            uncovered_callback(self.uncovered_statements)


@pytest.fixture(scope='session')
def sql_coverage(sql_files) -> Coverage:
    """
    Returns data about the current coverage of statements.

    See @ref sql_coverage_test_info "SQL coverage tests" for more info.

    @ingroup userver_testsuite_fixtures
    """
    return Coverage(set(sql_files))


@pytest.fixture(scope='function', autouse=True)
async def sql_statement_hook(testpoint, sql_coverage):
    """
    Hook that accepts requests from the testpoint with information on PostgreSQL statements coverage.

    See @ref sql_coverage_test_info "SQL coverage tests" for more info.

    @ingroup userver_testsuite_fixtures
    """

    @testpoint('sql_statement')
    def _hook(request):
        sql_coverage.cover(request['name'])

    yield _hook


@pytest.fixture(scope='function', autouse=True)
async def yql_statement_hook(testpoint, sql_coverage):
    """
    Hook that accepts requests from the testpoint with information on YDB statements coverage.

    See @ref sql_coverage_test_info "SQL coverage tests" for more info.

    @ingroup userver_testsuite_fixtures
    """

    @testpoint('yql_statement')
    def _hook(request):
        sql_coverage.cover(request['name'])

    yield _hook


@pytest.hookimpl(hookwrapper=True)
def pytest_collection_modifyitems(config, items):
    yield
    if not items:
        return

    coverage.collection_modifyitems(_SQL_COVERAGE_TEST_NAME, config, items)
