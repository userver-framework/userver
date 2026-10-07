import pytest
from pytest_userver.plugins import base

MAIN_PORT = 12001
MONITOR_PORT = 12002
ALLOCATED_PORT_OFFSET = 10000
PORT_HINTS = (8080, 8081, 8082)


@pytest.fixture(scope='session')
def service_port():
    return MAIN_PORT


@pytest.fixture(scope='session')
def monitor_port():
    return MONITOR_PORT


@pytest.fixture(scope='session')
def choose_free_port():
    def choose(port_hint):
        assert isinstance(port_hint, int)
        return port_hint + ALLOCATED_PORT_OFFSET

    return choose


@pytest.mark.parametrize('listener_name,first_port', [('listener', MAIN_PORT), ('listener-monitor', MONITOR_PORT)])
def test_all_listener_ports_are_allocated(userver_config_http_server, listener_name, first_port):
    tls = {'cert': 'cert.crt', 'private-key': 'private_key.key'}
    listener = {'ports': [{'port': PORT_HINTS[0]}, {'port': PORT_HINTS[1], 'tls': tls}, {'port': PORT_HINTS[2]}]}
    config = {'components_manager': {'components': {'server': {listener_name: listener}}}}

    userver_config_http_server(config, {})

    assert [entry['port'] for entry in listener['ports']] == [
        first_port,
        PORT_HINTS[1] + ALLOCATED_PORT_OFFSET,
        PORT_HINTS[2] + ALLOCATED_PORT_OFFSET,
    ]
    assert listener['ports'][1]['tls'] == tls
    assert 'port' not in listener


def test_first_listener_port_is_an_integer_hint(choose_free_port):
    class OriginalConfig:
        config_yaml = {
            'components_manager': {'components': {'server': {'listener': {'ports': [{'port': PORT_HINTS[0]}]}}}}
        }
        config_vars = {}

    assert base._get_port(OriginalConfig(), choose_free_port, 'listener', service_port, '--service-port') == (
        PORT_HINTS[0] + ALLOCATED_PORT_OFFSET
    )


@pytest.mark.parametrize('config_vars', [{'tls-port': PORT_HINTS[1]}, {}])
def test_extra_listener_port_resolves_variables(userver_config_http_server, config_vars):
    listener = {
        'ports': [{'port': PORT_HINTS[0]}, {'port': '$tls-port', 'port#fallback': PORT_HINTS[1]}],
    }
    config = {'components_manager': {'components': {'server': {'listener': listener}}}}

    userver_config_http_server(config, config_vars)

    assert listener['ports'][1]['port'] == PORT_HINTS[1] + ALLOCATED_PORT_OFFSET


@pytest.mark.parametrize('config_vars', [{'main-port': PORT_HINTS[0]}, {}])
def test_first_listener_port_resolves_variables(choose_free_port, config_vars):
    class OriginalConfig:
        config_yaml = {
            'components_manager': {
                'components': {
                    'server': {'listener': {'ports': [{'port': '$main-port', 'port#fallback': PORT_HINTS[0]}]}}
                }
            }
        }

    original_config = OriginalConfig()
    original_config.config_vars = config_vars
    assert base._get_port(original_config, choose_free_port, 'listener', service_port, '--service-port') == (
        PORT_HINTS[0] + ALLOCATED_PORT_OFFSET
    )
