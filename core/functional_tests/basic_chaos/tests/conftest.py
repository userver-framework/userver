import pytest


pytest_plugins = ['pytest_userver.plugins.core', 'pytest_userver.plugins']


USERVER_CONFIG_HOOKS = ['_userver_config_dns_link']


@pytest.fixture(name='for_client_gate_port', scope='module')
def _for_client_gate_port(request) -> int:
    # This fixture might be defined in an outer scope.
    if 'for_client_gate_port_override' in request.fixturenames:
        return request.getfixturevalue('for_client_gate_port_override')
    return 11433


@pytest.fixture(name='for_dns_gate_port', scope='session')
def _for_dns_gate_port(request) -> int:
    # This fixture might be defined in an outer scope.
    if 'for_dns_gate_port_override' in request.fixturenames:
        return request.getfixturevalue('for_dns_gate_port_override')
    return 11455


@pytest.fixture(scope='session')
def _userver_config_dns_link(for_dns_gate_port):
    def patch_config(config, _config_vars) -> None:
        components = config['components_manager']['components']

        component = 'handler-chaos-dns-resolver'
        if component in components:
            components[component]['dns-server'] = f'[::1]:{for_dns_gate_port}'

    return patch_config
