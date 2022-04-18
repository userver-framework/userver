async def test_config(service_client):
    response = await service_client.post('/configs/values', json={})
    assert response.status == 200
    reply = response.json()
    assert reply['configs']['USERVER_LOG_REQUEST_HEADERS'] is True
