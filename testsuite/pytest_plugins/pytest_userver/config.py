"""Supplementary types and functions for @ref pytest_userver.plugins.config."""

from collections.abc import Callable
import dataclasses

import testsuite.fixture_markers

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

    An override of a marked fixture must repeat this decorator.

    @snippet samples/static_service/testsuite/conftest.py  static config patch

    @ingroup userver_testsuite
    """
    return testsuite.fixture_markers.mark(func, _ConfigPatchInfo())
