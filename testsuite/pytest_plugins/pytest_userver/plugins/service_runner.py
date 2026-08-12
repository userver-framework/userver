"""
Helpers to make the `make start-*` commands work.
"""

from collections.abc import Iterable
from collections.abc import Sequence

import pytest

pytest_plugins = ['pytest_userver.plugins.generated_tests']


def pytest_generate_virtual_tests(
    parent: pytest.File,
    config: pytest.Config,
    existing_items: Sequence[pytest.Item],
) -> Iterable[pytest.Item]:
    if not config.option.service_runner_mode:
        return

    for item in existing_items:
        for marker in item.own_markers:
            if marker.name == 'servicetest':
                return

    yield pytest.Function.from_parent(
        parent=parent,
        name=test_service_default.__name__,
        callobj=test_service_default,
    )


@pytest.mark.servicetest
def test_service_default(
    service_client,
    service_baseurl,
    monitor_baseurl,
    request,
) -> None:
    """
    This is default service runner testcase. Feel free to override it
    in your own tests, e.g.:

    @code
    @pytest.mark.servicetest
    def test_service(service_client):
        ...
    @endcode

    @ingroup userver_testsuite
    """
    # TODO: use service_client.base_url() and monitor_client.base_url()
    delimiter = '=' * 100
    message = f'\n{delimiter}\nStarted service at {service_baseurl}'
    if monitor_baseurl:
        message += f'\nMonitor URL is {monitor_baseurl}'
    if 'grpc_service_endpoint' in request.fixturenames:
        grpc_endpoint = request.getfixturevalue('grpc_service_endpoint')
        message += f'\ngRPC endpoint is {grpc_endpoint}'
    message += f'\n{delimiter}\n'
    print(message)
