"""Supplementary types and functions for @ref pytest_userver.plugins.service."""

from collections.abc import Callable
import dataclasses

import testsuite.fixture_markers

# @cond


@dataclasses.dataclass(frozen=True)
class _ServiceDependencyInfo:
    pass


# @endcond


def dependency(func: Callable[..., object]) -> Callable[..., object]:
    """
    Mark a pytest fixture as a service dependency. This fixture will automatically
    be depended on by
    @ref pytest_userver.plugins.service.service_daemon_instance "service_daemon_instance"
    fixture.

    Apply to the original function, under `@pytest.fixture`.

    An override of a marked fixture must repeat this decorator.

    @snippet samples/http_caching/tests/conftest.py  service dependency

    @ingroup userver_testsuite
    """
    return testsuite.fixture_markers.mark(func, _ServiceDependencyInfo())
