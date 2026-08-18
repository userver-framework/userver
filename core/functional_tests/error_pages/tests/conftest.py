import pytest

pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['error_pages_config_hook']


@pytest.fixture(scope='session')
def error_pages_config_hook(service_source_dir):
    """Makes the 'body-path' of the error page absolute."""

    def _patch_config(config_yaml, config_vars):
        listener = config_yaml['components_manager']['components']['server']['listener']
        for page in listener['error-pages']:
            if 'body-path' in page:
                page['body-path'] = str(service_source_dir / page['body-path'])

    return _patch_config
