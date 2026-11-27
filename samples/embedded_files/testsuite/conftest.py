import pytest

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
def service_env(service_port):
    return {'EMBEDDED_SERVICE_PORT': str(service_port)}
