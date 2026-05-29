import pytest

pytest_plugins = ['pytest_userver.plugins.s3api']

USERVER_CONFIG_HOOKS = ['userver_s3_mock']


@pytest.fixture(scope='session')
def userver_s3_mock(mockserver_info):
    def _do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['s3-sample-component']['url'] = mockserver_info.url('/mds-s3')

    return _do_patch
