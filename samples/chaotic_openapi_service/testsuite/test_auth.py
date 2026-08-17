async def test_greeting_requires_auth(service_client):
    response = await service_client.get('/secure/greeting')
    assert response.status == 401


async def test_greeting_with_bad_token(service_client):
    response = await service_client.get('/secure/greeting', headers={'Authorization': 'not a bearer token'})
    assert response.status == 401


async def test_greeting_with_unknown_token(service_client):
    response = await service_client.get('/secure/greeting', headers={'Authorization': 'Bearer unknown-token'})
    assert response.status == 401


async def test_greeting_with_auth(service_client):
    response = await service_client.get('/secure/greeting', headers={'Authorization': 'Bearer user-1-token'})
    assert response.status == 200
    assert response.json()['greeting'] == 'Hello, user 123!'
