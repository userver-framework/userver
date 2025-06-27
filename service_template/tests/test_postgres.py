# postgresql template on
# Start the tests via `make test-debug` or `make test-release`

import pytest

from testsuite.databases import pgsql


async def test_basic(service_client):
    response = await service_client.post('/hello-postgres', params={'name': 'Tester'})
    assert response.status == 200
    assert response.text == 'Hello, Tester!\n'


async def test_db_updates(service_client):
    response = await service_client.post('/hello-postgres', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hello, World!\n'

    response = await service_client.post('/hello-postgres', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hi again, World!\n'

    response = await service_client.post('/hello-postgres', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hi again, World!\n'


@pytest.mark.pgsql('db_1', files=['initial_data.sql'])
async def test_db_initial_data(service_client):
    response = await service_client.post(
        '/hello-postgres',
        params={'name': 'user-from-initial_data.sql'},
    )
    assert response.status == 200
    assert response.text == 'Hi again, user-from-initial_data.sql!\n'


# postgresql template on
