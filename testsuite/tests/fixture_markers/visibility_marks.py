import dataclasses

import pytest
from pytest_userver import fixture_markers


@dataclasses.dataclass(frozen=True)
class VisibilityMark:
    origin: str


def visibility(origin: str):
    def decorator(func):
        return fixture_markers.mark(func, VisibilityMark(origin=origin))

    return decorator


class VisibilityPlugin:
    @pytest.fixture(scope='session')
    @visibility('plugin')
    def plugin_visibility(self):
        return 'plugin'
