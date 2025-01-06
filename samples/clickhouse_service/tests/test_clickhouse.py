async def test_clickhouse(service_client):
    response = await service_client.request('GET', '/v1/db?limit=10')
    assert response.status_code == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == '0123456789'
