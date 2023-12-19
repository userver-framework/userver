import pytest

import auth_utils


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_authenticate_base_proxy(service_client):
    response = await service_client.get('/v1/hello-proxy')
    assert response.status == 401

    authentication_header = response.headers['Proxy-Authenticate']
    auth_directives = auth_utils.parse_directives(authentication_header)

    auth_utils.auth_directives_assert(auth_directives)

    challenge = auth_utils.construct_challenge(auth_directives)
    auth_header = auth_utils.construct_header('username', 'pswd', challenge)

    response = await service_client.get(
        '/v1/hello-proxy', headers={'Proxy-Authorization': auth_header},
    )
    assert response.status == 200
    assert 'Proxy-Authentication-Info' in response.headers


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_postgres_wrong_data_proxy(service_client):
    response = await service_client.get('/v1/hello-proxy')
    assert response.status == 401

    authentication_header = response.headers['Proxy-Authenticate']
    auth_directives = auth_utils.parse_directives(authentication_header)

    auth_utils.auth_directives_assert(auth_directives)

    challenge = auth_utils.construct_challenge(auth_directives)
    auth_header = auth_utils.construct_header(
        'username', 'wrong-password', challenge,
    )

    response = await service_client.get(
        '/v1/hello-proxy', headers={'Proxy-Authorization': auth_header},
    )
    assert response.status == 401
    assert 'Proxy-Authenticate' in response.headers
