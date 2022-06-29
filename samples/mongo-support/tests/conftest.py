import pytest

pytest_plugins = [
    'pytest_userver.plugins',
    'testsuite.databases.mongo.pytest_plugin',
]

USERVER_CONFIG_HOOKS = ['mongo_config_hook']
MONGO_COLLECTIONS = {
    'distlocks': {
        'settings': {
            'collection': 'distlocks',
            'connection': 'sample',
            'database': 'sample',
        },
        'indexes': [],
    },
}


@pytest.fixture(scope='session')
def mongodb_settings():
    return MONGO_COLLECTIONS


@pytest.fixture
def client_deps(mongodb):
    pass


@pytest.fixture(scope='session')
def mongo_config_hook(mongo_connection_info):
    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['mongo-sample'][
            'dbconnection'
        ] = mongo_connection_info.get_uri('sample')

    return _patch_config
