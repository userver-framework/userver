async def test_monitor(monitor_client):
    response = await monitor_client.get('/service/log-level/')
    assert response.status == 200
