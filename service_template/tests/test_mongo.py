# mongo template on
# Start the tests via `make test-debug` or `make test-release`

import pytest

from testsuite.databases import mongo


async def test_basic(service_client):
    response = await service_client.post('/hello-mongo', params={'name': 'Tester'})
    assert response.status == 200
    assert response.text == 'Hello, Tester!\n'


async def test_db_updates(service_client):
    response = await service_client.post('/hello-mongo', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hello, World!\n'

    response = await service_client.post('/hello-mongo', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hi again, World!\n'

    response = await service_client.post('/hello-mongo', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hi again, World!\n'


# mongo template off
