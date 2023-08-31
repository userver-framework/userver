import pytest
import os
import sys

sys.path.append(os.path.dirname(__file__) + '/../utils')
from auth_utils import *

@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_authenticate_base_proxy(service_client):
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    authentication_header = response.headers["Proxy-Authenticate"]
    auth_directives = parse_directives(authentication_header)

    auth_directives_assert(auth_directives) 

    challenge = construct_challenge(auth_directives)
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Proxy-Authorization': auth_header},
    )
    assert response.status == 200
    assert 'Proxy-Authentication-Info' in response.headers


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_postgres_wrong_data_proxy(service_client):
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    authentication_header = response.headers["Proxy-Authenticate"]
    auth_directives = parse_directives(authentication_header)

    auth_directives_assert(auth_directives)

    challenge = construct_challenge(auth_directives)
    auth_header = construct_header("username", "wrong-password", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Proxy-Authorization': auth_header},
    )
    assert response.status == 401
    assert 'Proxy-Authenticate' in response.headers
