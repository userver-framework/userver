import pytest


pytest_plugins = ['pytest_userver.plugins.mongo']

MONGO_COLLECTIONS = {
    'translations': {
        'settings': {
            'collection': 'translations',
            'connection': 'admin',
            'database': 'admin',
        },
        'indexes': [],
    },
}


@pytest.fixture(scope='session')
def mongodb_settings():
    return MONGO_COLLECTIONS
