import pytest

from testsuite import utils


# /// [patch configs]
pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['userver_config_translations']


@pytest.fixture(scope='session')
def userver_config_translations(mockserver_info):
    def do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['cache-http-translations'][
            'translations-url'
        ] = mockserver_info.url('v1/translations')

    return do_patch
    # /// [patch configs]


# /// [mockserver]
@pytest.fixture(autouse=True)
def mock_translations(mockserver, translations, mocked_time):
    @mockserver.json_handler('/v1/translations')
    def mock(request):
        return {
            'content': translations,
            'update_time': utils.timestring(mocked_time.now()),
        }

    return mock
    # /// [mockserver]


# /// [translations]
@pytest.fixture
def translations():
    return {
        'hello': {'en': 'hello', 'ru': 'Привет'},
        'welcome': {'ru': 'Добро пожаловать', 'en': 'Welcome'},
    }
    # /// [translations]
