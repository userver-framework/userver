"""Supplementary types and functions for @ref pytest_userver.plugins.service."""

from collections.abc import Callable
import dataclasses

from pytest_userver import fixture_markers

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

    Overrides inherit this mark automatically; repeating the decorator is not
    required, even if the override does not request the original fixture.
    The dependency resolves to the overriding fixture, not the original one.

    @snippet samples/http_caching/tests/conftest.py  service dependency

    @see @ref fixture_markers_overrides "Fixture marker inheritance"

    @ingroup userver_testsuite
    """
    return fixture_markers.mark(func, _ServiceDependencyInfo())
