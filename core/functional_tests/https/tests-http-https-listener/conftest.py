import pytest

pytest_plugins = ['pytest_userver.plugins.core', 'pytest_userver.plugins']


@pytest.fixture(scope='session')
def port1(get_free_port):
    return get_free_port()


@pytest.fixture(scope='session')
def port2(get_free_port):
    return get_free_port()


@pytest.fixture(scope='session')
def service_baseurl(port1) -> str:
    return f'http://localhost:{port1}/'


@pytest.fixture(name='userver_config_http_client', scope='session')
def _userver_config_http_client(userver_config_http_client, port1, port2, service_source_dir):
    def patch_config(config_yaml, config_vars) -> None:
        userver_config_http_client(config_yaml, config_vars)

        components = config_yaml['components_manager']['components']

        # remove defaults from service static_config.yaml
        del components['server']['listener']['port']
        del components['server']['listener']['tls']

        components['server']['listener']['ports'] = [
            {'port': port1},
            {
                'port': port2,
                'tls': {
                    'cert': str(service_source_dir / 'cert.crt'),
                    'private-key': str(service_source_dir / 'private_key.key'),
                    'private-key-passphrase-name': 'tls',
                },
            },
        ]

        components['default-secdist-provider']['config'] = str(
            service_source_dir / 'secdist.json',
        )

    return patch_config
