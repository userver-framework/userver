from taxi_tests.daemons import service_client
from tests_plugins import userver_fixture


pytest_plugins = [
    'tests_plugins',
]


def _please_implement_tests_control(baseurl, **kwargs):
    kwargs.pop('now', None)
    return service_client.ServiceClient(baseurl, **kwargs)


taxi_driver_authorizer = userver_fixture.create_client_fixture(
    'yandex-taxi-driver-authorizer',
    client_class=_please_implement_tests_control
)
