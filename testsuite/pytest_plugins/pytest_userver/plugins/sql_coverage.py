"""
Plugin that imports the required fixtures for checking SQL/YQL coverage. See
@ref sql_coverage_test_info "SQL coverage tests" for more info.

@ingroup userver_testsuite_fixtures
"""

from collections.abc import Iterable
from collections.abc import Sequence

import pytest

import pytest_userver.utils.coverage

pytest_plugins = ['pytest_userver.plugins.generated_tests']


@pytest.fixture
def on_uncovered():
    """
    Called when the coverage is incomplete.

    Override this fixture to change the way uncovered statements are reported or to ignore some of the statements
    from coverage report.

    See @ref sql_coverage_test_info "SQL coverage tests" for more info.

    @ingroup userver_testsuite_fixtures
    """

    def _on_uncovered(uncovered_statements: set[str]):
        msg = f'Uncovered SQL/YQL statements: {uncovered_statements}'
        raise pytest_userver.utils.coverage.UncoveredError(msg)

    return _on_uncovered


class Coverage:
    """
    Contains data about the current coverage of statements.
    """

    def __init__(self, files: set[str]):
        self.covered_statements: set[str] = set()
        self.uncovered_statements: set[str] = files
        self.extra_covered_statements: set[str] = set()

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


@pytest.fixture(autouse=True)
def sql_statement_hook(testpoint, sql_coverage):
    """
    Hook that accepts requests from the testpoint with information on PostgreSQL statements coverage.

    See @ref sql_coverage_test_info "SQL coverage tests" for more info.

    @ingroup userver_testsuite_fixtures
    """

    @testpoint('sql_statement')
    def _hook(request):
        sql_coverage.cover(request['name'])

    return _hook


@pytest.fixture(autouse=True)
async def yql_statement_hook(testpoint, sql_coverage):
    """
    Hook that accepts requests from the testpoint with information on YDB statements coverage.

    See @ref sql_coverage_test_info "SQL coverage tests" for more info.

    @ingroup userver_testsuite_fixtures
    """

    @testpoint('yql_statement')
    def _hook(request):
        sql_coverage.cover(request['name'])

    return _hook


def test_sql_coverage(sql_coverage, on_uncovered):
    sql_coverage.validate(on_uncovered)


def pytest_generate_virtual_tests(
    parent: pytest.File,
    config: pytest.Config,
    existing_items: Sequence[pytest.Item],
) -> Iterable[pytest.Item]:
    if config.pluginmanager.hasplugin('sql_files'):
        yield pytest.Function.from_parent(
            parent=parent,
            name=test_sql_coverage.__name__,
            callobj=test_sql_coverage,
        )


@pytest.hookimpl(wrapper=True)
def pytest_collection_modifyitems(config: pytest.Config, items: list[pytest.Item]) -> None:
    yield
    if items:
        pytest_userver.utils.coverage.collection_modifyitems(test_sql_coverage.__name__, config, items)
