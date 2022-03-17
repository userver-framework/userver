async def test_monitor(test_monitor_client):
    response = await test_monitor_client.get('/service/log-level/')
    assert response.status == 200
