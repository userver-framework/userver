"""
Provides a nicer pytest diff (via `testsuite`'s `CompareVisitor` mechanism)
for failing `== ` comparisons that involve
@ref pytest_userver.metrics.MetricsSnapshot "MetricsSnapshot".

@ingroup userver_testsuite_fixtures
"""

from typing import Any

from testsuite.plugins.assertrepr_compare import CompareVisitor

import pytest_userver.metrics


def pytest_register_compare_visitors() -> list[CompareVisitor]:
    return [
        CompareVisitor(
            predicate=_is_metrics_comparison,
            visit=_metrics_compare_visit,
        ),
    ]


def _is_metrics_comparison(left: Any, right: Any) -> bool:
    return any(isinstance(operand, pytest_userver.metrics.MetricsSnapshot) for operand in (left, right))


def _metrics_compare_visit(left: Any, right: Any, reporter: object) -> tuple[Any, Any]:
    return (
        pytest_userver.metrics.stringify_snapshot_for_diff(left, other=right),
        pytest_userver.metrics.stringify_snapshot_for_diff(right, other=left),
    )
