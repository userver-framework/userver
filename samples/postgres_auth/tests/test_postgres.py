# /// [Functional test]
import pytest


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_postgres(service_client):
    response = await service_client.get('/v1/hello')
    assert response.status == 401
    assert response.content == b'Empty \'Authorization\' header'

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': 'Bearer THE_USER_TOKEN'},
    )
    assert response.status == 200
    assert response.content == b'Hello world, Dear User!\n'
    # /// [Functional test]

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': 'wrong format'},
    )
    assert response.status == 401
    assert b'Bearer some-token' in response.content

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': 'wrong'},
    )
    assert response.status == 401
    assert b'Bearer some-token' in response.content

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': 'Bearer wrong-token'},
    )
    assert response.status == 403

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': 'Bearer '},
    )
    assert response.status == 403

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': 'Bearer wrong-scopes-token'},
    )
    assert response.status == 403
