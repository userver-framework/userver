import pytest

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
def allowed_url_prefixes_extra(service_port):
    """Allow WebSocket connections to the test service."""
    return [f'ws://localhost:{service_port}/']
