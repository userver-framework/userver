"""
Makes sure that the service started via the `start-*` cmake target serves
requests, performing the same key-value request as the neighboring `tests/`
tests (but without direct database access, which is owned by the inner
service-runner-mode session).
"""


async def test_basic_via_start_target(service_client):
    response = await service_client.post('/v1/key-value?key=start_target&value=world')
    assert response.status == 201
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'world'

    response = await service_client.get('/v1/key-value?key=start_target')
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'world'
