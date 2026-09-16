"""The client generated from openapi.yaml authenticates itself with HTTP digest."""

import pytest
from pytest_userver.utils import httpdigest

MOCK_REALM = 'mockrealm@userver.com'
MOCK_NONCE = 'dcd98b7102dd2f0e8b11d0f600bfb0c093'
MOCK_QOP = 'auth'
# Must match http_digest.secure.myDigestScheme in secure_data.json.
MOCK_USERNAME = 'alice'
MOCK_PASSWORD = 'alice-password'


class DigestServerMock:
    """Records the digest-authenticated requests seen by the mock."""

    def __init__(self):
        self.authenticated_requests = []


# /// [Digest server mock]
@pytest.fixture(name='digest_server')
def _digest_server(mockserver):
    state = DigestServerMock()

    def _make_challenge():
        return mockserver.make_response(
            status=401,
            headers={
                'WWW-Authenticate': (
                    f'Digest realm="{MOCK_REALM}", nonce="{MOCK_NONCE}", qop="{MOCK_QOP}", algorithm=MD5'
                ),
            },
        )

    @mockserver.handler('/api/secure/secret')
    def _handler(request):
        auth_header = request.headers.get('Authorization')
        if auth_header is None:
            return _make_challenge()

        directives = httpdigest.parse_directives(auth_header)
        state.authenticated_requests.append(directives)

        expected_response = httpdigest.calculate_response(
            username=MOCK_USERNAME,
            password=MOCK_PASSWORD,
            realm=MOCK_REALM,
            nonce=MOCK_NONCE,
            nonce_count=directives['nc'],
            cnonce=directives['cnonce'],
            qop=MOCK_QOP,
            method=request.method,
            uri=directives['uri'],
        )
        if directives['username'] != MOCK_USERNAME or directives['response'] != expected_response:
            return mockserver.make_response(status=403, json={'error': 'invalid digest'})

        # Redirect within the same request to verify that libcurl reuses the
        # Digest session and increments its nonce count.
        if len(state.authenticated_requests) == 1:
            return mockserver.make_response(
                status=302,
                headers={'Location': '/api/secure/secret'},
            )

        return mockserver.make_response(json={'greeting': 'Hello from the digest-protected API!'})

    return state
    # /// [Digest server mock]


# /// [Client functional test]
async def test_client_authenticates_with_digest(service_client, digest_server):
    response = await service_client.get('/call-secret')
    assert response.status == 200
    assert response.text == 'Hello from the digest-protected API!'

    # The credentials came from secdist, not from the handler code.
    assert len(digest_server.authenticated_requests) == 2
    for nonce_count, directives in enumerate(digest_server.authenticated_requests, start=1):
        assert directives['username'] == MOCK_USERNAME
        assert directives['realm'] == MOCK_REALM
        assert directives['qop'] == MOCK_QOP
        assert directives['nc'] == f'{nonce_count:08d}'
    # /// [Client functional test]
