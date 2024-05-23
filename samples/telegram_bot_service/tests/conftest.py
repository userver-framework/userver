import pytest


pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['prepare_service_config']

BOT_TOKEN = "123456:ABC-DEF1234ghIkl-zyx57W2v1u123ew11"


def ensure_no_trailing_separator(url: str) -> str:
    if url.endswith('/'):
        return url[:-1]
    return url


@pytest.fixture(scope='session')
def prepare_service_config(mockserver_info):
    def do_patch(config, config_vars):
        components = config['components_manager']['components']
        components['telegram-bot-client']['bot-token'] = BOT_TOKEN
        components['telegram-bot-client']['api-base-url'] = \
            ensure_no_trailing_separator(mockserver_info.base_url)

    return do_patch


@pytest.fixture(autouse=True)
def mock_get_updates(mockserver, updates, tg_method):
    @mockserver.json_handler(tg_method('getUpdates'))
    def get_updates_mock(*, body_json):
        offset = body_json['offset']
        limit = body_json['limit']

        result_updates = []
        for update in updates:
          if len(result_updates) == limit:
            break
          if update['update_id'] >= offset:
            result_updates.append(update)

        return {
          'ok': True,
          'result': result_updates
        }
    return get_updates_mock


@pytest.fixture
def updates():
    return []


@pytest.fixture
def tg_method():
    def get_tg_method(method):
        return '/bot{}/{}'.format(BOT_TOKEN, method)
    
    return get_tg_method
