# /// [service_non_http_health_checker]
import pytest

from pytest_userver.utils import net


pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
def tcp_service_port(service_config_yaml) -> int:
    components = service_config_yaml['components_manager']['components']
    tcp_hello = components.get('tcp-hello')
    assert tcp_hello, 'No "tcp-hello" component found'
    return int(tcp_hello['port'])


@pytest.fixture(scope='session')
def service_non_http_health_checks(
        service_config_yaml, tcp_service_port,
) -> net.HealthChecks:
    checks = net.get_health_checks_info(service_config_yaml)
    checks.tcp.append(net.HostPort(host='localhost', port=tcp_service_port))
    return checks
    # /// [service_non_http_health_checker]
