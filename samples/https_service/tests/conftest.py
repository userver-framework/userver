import pathlib

import pytest


# /// [registration]
pytest_plugins = ['pytest_userver.plugins.core']
# /// [registration]

USERVER_CONFIG_HOOKS = ['prepare_service_config']
SERVICE_SOURCE_DIR = pathlib.Path(__file__).parent.parent


@pytest.fixture(scope='session')
def prepare_service_config():
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        tls = components['server']['listener']['tls']
        tls['ca'] = [str(SERVICE_SOURCE_DIR / 'cert.crt')]
        tls['cert'] = str(SERVICE_SOURCE_DIR / 'cert.crt')
        tls['private-key'] = str(SERVICE_SOURCE_DIR / 'private_key.key')

        components['default-secdist-provider']['config'] = str(
            SERVICE_SOURCE_DIR / 'secdist.json',
        )

    return patch_config
