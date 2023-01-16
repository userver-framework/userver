import pytest

pytest_plugins = ['pytest_userver.plugins.mongo']

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
