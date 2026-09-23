# /// [patch configs]
import pytest
import pytest_userver.config
import pytest_userver.service

from testsuite import utils

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
@pytest_userver.config.patch
def userver_config_translations(mockserver_info):
    def do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['cache-http-translations']['translations-url'] = mockserver_info.url('v1/translations')

    return do_patch
    # /// [patch configs]


# /// [service dependency]
@pytest.fixture
@pytest_userver.service.dependency
def mock_translations(mockserver, translations, mocked_time):
    @mockserver.json_handler('/v1/translations')
    def mock(request):
        return {
            'content': translations,
            'update_time': utils.timestring(mocked_time.now()),
        }

    return mock
    # /// [service dependency]


# /// [translations]
@pytest.fixture(name='translations')
def _translations():
    return {
        'hello': {'en': 'hello', 'ru': 'Привет'},
        'welcome': {'ru': 'Добро пожаловать', 'en': 'Welcome'},
    }
    # /// [translations]
