"""The handler generated from openapi.yaml is protected by HTTP digest auth."""

from pytest_userver.utils import httpdigest

URI = '/secure/secret'
REALM = 'chaotic@userver.com'


# /// [Handler functional test]
async def test_handler_challenges_unauthenticated_request(service_client):
    response = await service_client.get(URI)
    assert response.status == 401

    directives = httpdigest.parse_directives(response.headers['WWW-Authenticate'])
    assert directives['realm'] == REALM
    assert 'nonce' in directives
    assert directives['algorithm'] == 'MD5'
    assert directives['qop'] == 'auth'


async def test_handler_accepts_valid_credentials(service_client):
    response = await service_client.get(URI)
    assert response.status == 401

    challenge = httpdigest.construct_challenge(
        auth_directives=httpdigest.parse_directives(response.headers['WWW-Authenticate'])
    )
    digest_client = httpdigest.DigestAuthClient(username='alice', password='alice-password')

    first_auth_header = digest_client.construct_header(challenge=challenge, uri=URI, method='GET')
    assert httpdigest.parse_directives(first_auth_header)['nc'] == '00000001'

    auth_header = digest_client.construct_header(challenge=challenge, uri=URI, method='GET')
    assert httpdigest.parse_directives(auth_header)['nc'] == '00000002'

    response = await service_client.get(URI, headers={'Authorization': auth_header})
    assert response.status == 200
    assert response.json() == {'greeting': 'Hello, authenticated user!'}
    assert 'Authentication-Info' in response.headers
    # /// [Handler functional test]


async def test_handler_rejects_wrong_password(service_client):
    response = await service_client.get(URI)
    assert response.status == 401

    challenge = httpdigest.construct_challenge(
        auth_directives=httpdigest.parse_directives(response.headers['WWW-Authenticate'])
    )
    auth_header = httpdigest.construct_header(
        username='alice', password='wrong-password', challenge=challenge, uri=URI, method='GET'
    )

    response = await service_client.get(URI, headers={'Authorization': auth_header})
    assert response.status == 401
    assert 'WWW-Authenticate' in response.headers


async def test_handler_rejects_unregistered_user(service_client):
    response = await service_client.get(URI)
    assert response.status == 401

    challenge = httpdigest.construct_challenge(
        auth_directives=httpdigest.parse_directives(response.headers['WWW-Authenticate'])
    )
    auth_header = httpdigest.construct_header(
        username='bob', password='alice-password', challenge=challenge, uri=URI, method='GET'
    )

    response = await service_client.get(URI, headers={'Authorization': auth_header})
    assert response.status == 403
