from taxi_tests.daemons import service_client
from tests_plugins import userver_fixture


pytest_plugins = [
    'tests_plugins',
]

taxi_driver_authorizer = userver_fixture.create_client_fixture(
    'yandex-taxi-driver-authorizer'
)
