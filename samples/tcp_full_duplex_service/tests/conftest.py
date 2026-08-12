# /// [service_non_http_health_checker]
import pytest
from pytest_userver.utils import net

pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['userver_config_tcp_port']


@pytest.fixture(scope='session')
def userver_config_tcp_port(choose_free_port):
    def patch_config(config, _config_vars) -> None:
        components = config['components_manager']['components']
        tcp_echo = components['tcp-echo']
        tcp_echo['port'] = choose_free_port(tcp_echo['port'])

    return patch_config


@pytest.fixture(name='tcp_service_port', scope='session')
def _tcp_service_port(service_config) -> int:
    components = service_config['components_manager']['components']
    tcp_echo = components.get('tcp-echo')
    assert tcp_echo, 'No "tcp-echo" component found'
    return int(tcp_echo['port'])


@pytest.fixture(scope='session')
def service_non_http_health_checks(
    service_config,
    tcp_service_port,
) -> net.HealthChecks:
    checks = net.get_health_checks_info(service_config)
    checks.tcp.append(net.HostPort(host='localhost', port=tcp_service_port))
    return checks
    # /// [service_non_http_health_checker]
