async def test_undeclared_error_uses_policy(service_client):
    # The operation declares no 400, the policy of the handlers target renders it
    response = await service_client.post('/insecure/secret', params={'value': 'x'})
    assert response.status == 400
    assert response.headers['Content-Type'] == 'application/json'
    body = response.json()
    assert set(body) == {'code', 'message'}
    assert body['message']
