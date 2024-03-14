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

        components['default-secdist-provider']['config'] = str(
            service_source_dir / 'secdist.json',
        )

    return patch_config


@pytest.fixture(scope='session')
def service_baseurl(service_port) -> str:
    return f'https://localhost:{service_port}/'


@pytest.fixture(scope='session')
def service_client_session_factory(event_loop, service_source_dir):
    def make_session(**kwargs):
        kwargs.setdefault('loop', event_loop)
        kwargs['connector'] = aiohttp.TCPConnector(verify_ssl=False)
        return aiohttp.ClientSession(**kwargs)

    return make_session
