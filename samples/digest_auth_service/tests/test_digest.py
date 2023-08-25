# /// [Functional test]
import pytest
from requests.auth import HTTPDigestAuth
import re


# parsing regex
reg = re.compile(r'(\w+)[:=][\s"]?([^",]+)"?')


# construct challenge structer
def construct_challenge(auth_directives: dict, nonce=''):
    return {'realm': auth_directives["realm"],
            'nonce': auth_directives['nonce'] if len(nonce) == 0 else nonce,
            'algorithm': auth_directives["algorithm"],
            'qop': "auth"
            }


# construct authorization header sent from client
def construct_header(username: str, password: str, challenge: dict):
    digest_auth = HTTPDigestAuth(username, password)
    digest_auth.init_per_thread_state()
    digest_auth._thread_local.chal = challenge
    return digest_auth.build_digest_header('GET', '/v1/hello')


def parse_directives(authentication_header: str):
    return dict(reg.findall(authentication_header))


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_authenticate_base(service_client):
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    authentication_header = response.headers["WWW-Authenticate"]
    auth_directives = parse_directives(authentication_header)

    assert 'realm' in auth_directives
    assert 'nonce' in auth_directives
    assert 'algorithm' in auth_directives
    assert 'qop' in auth_directives

    challenge = construct_challenge(auth_directives)
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200
    assert 'Authentication-Info' in response.headers
# /// [Functional test]


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_postgres_wrong_data(service_client):
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    authentication_header = response.headers["WWW-Authenticate"]
    auth_directives = parse_directives(authentication_header)

    assert 'realm' in auth_directives
    assert 'nonce' in auth_directives
    assert 'algorithm' in auth_directives
    assert 'qop' in auth_directives

    challenge = construct_challenge(auth_directives)
    auth_header = construct_header("username", "WRONG-PASSWORD", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 401
    assert 'WWW-Authenticate' in response.headers


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_repeated_auth(service_client):
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    authentication_header = response.headers["WWW-Authenticate"]
    auth_directives = parse_directives(authentication_header)

    assert 'realm' in auth_directives
    assert 'nonce' in auth_directives
    assert 'algorithm' in auth_directives
    assert 'qop' in auth_directives

    challenge = construct_challenge(auth_directives)
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200

    auth_info_header = response.headers["Authentication-Info"]
    auth_directives_info = parse_directives(auth_info_header)

    challenge = construct_challenge(
        auth_directives, auth_directives_info['nextnonce'])
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200
    assert 'Authentication-Info' in response.headers


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_same_nonce_repeated_use(service_client):
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    authentication_header = response.headers["WWW-Authenticate"]
    auth_directives = parse_directives(authentication_header)

    assert 'realm' in auth_directives
    assert 'nonce' in auth_directives
    assert 'algorithm' in auth_directives
    assert 'qop' in auth_directives

    challenge = construct_challenge(auth_directives)
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200

    auth_info_header = response.headers["Authentication-Info"]
    auth_directives_info = parse_directives(auth_info_header)

    challenge = construct_challenge(
        auth_directives, auth_directives_info['nextnonce'])
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 401
    assert 'WWW-Authenticate' in response.headers


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_expiring_nonce(service_client, mocked_time):
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    authentication_header = response.headers["WWW-Authenticate"]
    auth_directives = parse_directives(authentication_header)

    assert 'realm' in auth_directives
    assert 'nonce' in auth_directives
    assert 'algorithm' in auth_directives
    assert 'qop' in auth_directives

    challenge = construct_challenge(auth_directives)
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200

    very_long_waiting_ms = 1500
    mocked_time.sleep(very_long_waiting_ms)

    auth_info_header = response.headers["Authentication-Info"]
    auth_directives_info = parse_directives(auth_info_header)

    challenge = construct_challenge(
        auth_directives, auth_directives_info['nextnonce'])
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 401

    authentication_header = response.headers["WWW-Authenticate"]
    auth_directives = parse_directives(authentication_header)

    assert 'realm' in auth_directives
    assert 'nonce' in auth_directives
    assert 'algorithm' in auth_directives
    assert 'qop' in auth_directives

    challenge = construct_challenge(auth_directives)
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200
    assert 'Authentication-Info' in response.headers


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_aliving_nonce_after_half_ttl(service_client, mocked_time):
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    authentication_header = response.headers["WWW-Authenticate"]
    auth_directives = parse_directives(authentication_header)

    assert 'realm' in auth_directives
    assert 'nonce' in auth_directives
    assert 'algorithm' in auth_directives
    assert 'qop' in auth_directives

    challenge = construct_challenge(auth_directives)
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200

    short_waiting_ms = 500
    mocked_time.sleep(short_waiting_ms)

    auth_info_header = response.headers["Authentication-Info"]
    auth_directives_info = parse_directives(auth_info_header)

    challenge = construct_challenge(
        auth_directives, auth_directives_info['nextnonce'])
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200
    assert 'Authentication-Info' in response.headers


@pytest.mark.pgsql('auth', files=['test_data.sql'])
async def test_repeated_auth_ignore_nextnonce(service_client):
    response = await service_client.get('/v1/hello')
    assert response.status == 401

    authentication_header = response.headers["WWW-Authenticate"]
    auth_directives = parse_directives(authentication_header)

    assert 'realm' in auth_directives
    assert 'nonce' in auth_directives
    assert 'algorithm' in auth_directives
    assert 'qop' in auth_directives

    challenge = construct_challenge(auth_directives)
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200

    response = await service_client.get('/v1/hello')
    assert response.status == 401

    authentication_header = response.headers["WWW-Authenticate"]
    auth_directives = parse_directives(authentication_header)

    challenge = construct_challenge(auth_directives)
    auth_header = construct_header("username", "pswd", challenge)

    response = await service_client.get(
        '/v1/hello', headers={'Authorization': auth_header},
    )
    assert response.status == 200
