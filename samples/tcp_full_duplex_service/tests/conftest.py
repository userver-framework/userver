# /// [service_non_http_health_checker]
import pytest

from pytest_userver.utils import net


@pytest.fixture(scope='session')
def service_non_http_health_checks(service_config_yaml) -> net.HealthChecks:
    checks = net.get_health_checks_info(service_config_yaml)

    components = service_config_yaml['components_manager']['components']
    tcp_hello = components.get('tcp-echo')
    assert tcp_hello, 'No "tcp-echo" component found'
    checks.tcp.append(net.HostPort(host='localhost', port=tcp_hello['port']))

    return checks
    # /// [service_non_http_health_checker]


pytest_plugins = ['pytest_userver.plugins', 'pytest_userver.plugins.samples']
