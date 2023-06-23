import pytest

pytest_plugins = ['pytest_userver.plugins.core', 'pytest_userver.plugins']

USERVER_CONFIG_HOOKS = ['_userver_config_dns_link']


@pytest.fixture(scope='session')
def dynamic_config_fallback_patch():
    return {
        'USERVER_HANDLER_STREAM_API_ENABLED': True,
        'USERVER_LOG_DYNAMIC_DEBUG': {
            'force-enabled': ['garbage'],
            'force-disabled': [],
        },
    }


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


@pytest.fixture(name='userver_config_http_client', scope='session')
def _userver_config_http_client(userver_config_http_client):
    def patch_config(config_yaml, config_vars) -> None:
        userver_config_http_client(config_yaml, config_vars)

        components = config_yaml['components_manager']['components']
        http_client = components['http-client']
        # There are tests in this module that specifically want to force
        # http-client timeouts.
        http_client.pop('testsuite-timeout')
        prefixes = http_client['testsuite-allowed-url-prefixes']
        # HACK: we'd like to write 'for_client_gate_port' here, but it has to
        # have 'module' scope. So we just allow the tests to go anywhere.
        prefixes.append('http://localhost')

    return patch_config
