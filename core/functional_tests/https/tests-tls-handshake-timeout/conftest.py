import ssl

import aiohttp
import pytest

pytest_plugins = ['pytest_userver.plugins.core']
USERVER_CONFIG_HOOKS = ['userver_config_tls_handshake']

KEEPALIVE_TIMEOUT_SECONDS = 1


@pytest.fixture(scope='session')
def userver_config_tls_handshake(service_source_dir):
    def patch_config(config_yaml, config_vars) -> None:
        components = config_yaml['components_manager']['components']

        listener = components['server']['listener']
        listener['connection'] = {'keepalive_timeout': KEEPALIVE_TIMEOUT_SECONDS}

        tls = listener['tls']
        tls['cert'] = str(service_source_dir / 'cert.crt')
        tls['private-key'] = str(service_source_dir / 'private_key.key')

        components['default-secdist-provider']['config'] = str(
            service_source_dir / 'secdist.json',
        )

    return patch_config


@pytest.fixture(scope='session')
def service_baseurl(service_port) -> str:
    return f'https://localhost:{service_port}/'


@pytest.fixture(scope='session')
def service_client_session_factory(service_source_dir):
    def make_session(**kwargs):
        ssl_context = ssl.create_default_context()
        ssl_context.load_verify_locations(str(service_source_dir / 'cert.crt'))
        kwargs['connector'] = aiohttp.TCPConnector(ssl=ssl_context)
        return aiohttp.ClientSession(**kwargs)

    return make_session
