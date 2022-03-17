import pytest

from testsuite import utils


@pytest.fixture(scope='session')
def patch_service_yaml(mockserver_info):
    def do_patch(config_yaml):
        components = config_yaml['components_manager']['components']
        components['cache-http-translations'][
            'translations-url'
        ] = mockserver_info.url('v1/translations')

    return do_patch


@pytest.fixture(autouse=True)
def mock_translations(mockserver, translations, mocked_time):
    @mockserver.json_handler('/v1/translations')
    def mock(request):
        return {
            'content': translations,
            'update_time': utils.timestring(mocked_time.now()),
        }

    return mock


@pytest.fixture
def translations():
    return {
        'hello': {'en': 'hello', 'ru': 'Привет'},
        'welcome': {'ru': 'Добро пожаловать', 'en': 'Welcome'},
    }
