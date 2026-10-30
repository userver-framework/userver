"""Supplementary types and functions for @ref pytest_userver.plugins.config."""

from collections.abc import Callable
import dataclasses

from pytest_userver import fixture_markers

# @cond


@dataclasses.dataclass(frozen=True)
class _ConfigPatchInfo:
    pass


# @endcond


def patch(func: Callable[..., object]) -> Callable[..., object]:
    """
    Mark a pytest fixture as a static config patch. The fixture will automatically
    be applied to the service config.

    Apply to the original function, under `@pytest.fixture`.

    Overrides inherit this mark automatically; repeating the decorator is not
    required, even if the override does not request the original fixture.
    The override keeps the original marked fixture's position in patch order.

    @snippet samples/static_service/testsuite/conftest.py  static config patch

    @see @ref fixture_markers_overrides "Fixture marker inheritance"

    @ingroup userver_testsuite
    """
    return fixture_markers.mark(func, _ConfigPatchInfo())
