import pytest

pytest_plugins = ['pytest_userver.plugins.mongo']

MONGO_COLLECTIONS = {
    'cached_documents': {
        'settings': {
            'collection': 'cached_documents',
            'connection': 'admin',
            'database': 'admin',
        },
        'indexes': [],
    },
}

DOCUMENTS = [
    {'key': 'first', 'value': 1},
    {'key': 'second', 'value': 2},
    {'key': 'third', 'value': 3},
]


@pytest.fixture(scope='session')
def mongodb_settings():
    return MONGO_COLLECTIONS


@pytest.fixture(name='cached_documents')
def _cached_documents(mongodb):
    collection = mongodb.cached_documents
    collection.delete_many({})
    # insert_many adds '_id' to the documents it is given, so pass copies
    collection.insert_many([dict(document) for document in DOCUMENTS])
    return collection
