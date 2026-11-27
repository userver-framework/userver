import pytest
from pytest_userver.plugins.config import _UserverConfigPlugin

from fixture_markers.guarantees import GuaranteesPlugin
from fixture_markers.guarantees import LaterPlugin
from fixture_markers.guarantees import ordered
from fixture_markers.guarantees import other
from fixture_markers.visibility_marks import VisibilityPlugin


def pytest_configure(config):
    if config.pluginmanager.has_plugin('visibility_plugin'):
        return
    config.pluginmanager.register(VisibilityPlugin(), 'visibility_plugin')
    config.pluginmanager.register(GuaranteesPlugin(), 'fixture_marker_guarantees')
    # Registered deliberately after GuaranteesPlugin to test cross-plugin ordering.
    config.pluginmanager.register(LaterPlugin(), 'fixture_marker_later')
    # Register only the hook collector, not the service-launching core plugin.
    config.pluginmanager.register(_UserverConfigPlugin(), 'userver_config')


@pytest.fixture(scope='session')
@ordered('conftest-first')
def z_conftest():
    return 'conftest-first'


@pytest.fixture(scope='session')
@ordered('nearest')
@other
def overridden():
    pytest.fail('Discovery must not execute the intermediate fixture')


@pytest.fixture(scope='session')
@ordered('conftest-last')
def a_conftest():
    return 'conftest-last'
