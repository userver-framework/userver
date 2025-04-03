import aiohttp
import pytest

pytest_plugins = ['pytest_userver.plugins.core']
USERVER_CONFIG_HOOKS = ['userver_config_secdist']


@pytest.fixture(scope='session')
def userver_config_secdist(userver_config_http_client, service_source_dir):
    def patch_config(config_yaml, config_vars) -> None:
        components = config_yaml['components_manager']['components']

        tls = components['server']['listener']['tls']
        tls['cert'] = str(service_source_dir / 'cert.crt')
        tls['private-key'] = str(service_source_dir / 'private_key.key')

        if 'config' in components['default-secdist-provider']:
            del components['default-secdist-provider']['config']
        components['default-secdist-provider']['inline'] = {
            'passphrases': {'tls': 'asd'},
        }

    return patch_config


@pytest.fixture(scope='session')
def service_baseurl(service_port) -> str:
    return f'https://localhost:{service_port}/'


@pytest.fixture(scope='session')
def service_client_session_factory(service_source_dir):
    def make_session(**kwargs):
        kwargs['connector'] = aiohttp.TCPConnector(verify_ssl=False)
        return aiohttp.ClientSession(**kwargs)

    return make_session
