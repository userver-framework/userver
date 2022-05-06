import pytest

# /// [mongodb settings]
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
    # /// [mongodb settings]


# /// [require mongodb]
@pytest.fixture
def client_deps(mongodb):
    pass
    # /// [require mongodb]
