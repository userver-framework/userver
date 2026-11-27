"""The client generated from openapi.yaml authenticates itself with HTTP Bearer."""

import pytest

# Must match http_bearer.bearer.myBearerScheme in secure_data.json.
MOCK_TOKEN = 'user-1-token'


class BearerServerMock:
    """Records the bearer-authenticated requests seen by the mock."""

    def __init__(self):
        self.authenticated_requests = []


@pytest.fixture(name='bearer_server')
def _bearer_server(mockserver):
    state = BearerServerMock()

    @mockserver.handler('/api/secure/bearer')
    def _handler(request):
        auth_header = request.headers.get('Authorization')
        state.authenticated_requests.append(auth_header)
        if auth_header != f'Bearer {MOCK_TOKEN}':
            return mockserver.make_response(status=401, json={'error': 'invalid bearer token'})
        return mockserver.make_response(json={'greeting': 'Hello from the bearer-protected API!'})

    return state


async def test_client_authenticates_with_bearer(service_client, bearer_server):
    response = await service_client.get('/call-bearer')
    assert response.status == 200
    assert response.text == 'Hello from the bearer-protected API!'

    # The credentials came from secdist, not from the handler code.
    assert bearer_server.authenticated_requests == [f'Bearer {MOCK_TOKEN}']
