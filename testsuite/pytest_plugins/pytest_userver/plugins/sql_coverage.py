from typing import Set
import warnings

import pytest

from . import coverage

_SQL_COVERAGE_TEST_NAME = 'test_sql_coverage'


@pytest.fixture
def on_uncovered(pytestconfig):
    def _on_uncovered(uncovered_statements: Set[str]):
        msg = f'Uncovered SQL statements: {uncovered_statements}'
        mode = pytestconfig.option.sql_coverage_notification_mode
        if mode == coverage.NotificationMode.WARNING:
            warnings.warn(msg)
        elif mode == coverage.NotificationMode.ERROR:
            raise coverage.UncoveredEndpointsError(msg)

    return _on_uncovered


class Coverage:
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
def sql_coverage(sql_files):
    return Coverage(set(sql_files))


@pytest.fixture(scope='function', autouse=True)
async def sql_statement_hook(testpoint, sql_coverage):
    @testpoint('sql_statement')
    def _hook(request):
        sql_coverage.cover(request['name'])

    yield _hook


@pytest.fixture(scope='function', autouse=True)
async def yql_statement_hook(testpoint, sql_coverage):
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


def pytest_addoption(parser) -> None:
    group = parser.getgroup('services')
    group.addoption(
        '--sql-coverage-notification-mode',
        dest='sql_coverage_notification_mode',
        type=coverage.NotificationMode,
        default=coverage.NotificationMode.SILENT,
        choices=[
            coverage.NotificationMode.SILENT,
            coverage.NotificationMode.WARNING,
            coverage.NotificationMode.ERROR,
        ],
        help=('Notification mode for SQL Coverage check. Might be: - "silent", "warning", "error"'),
    )
