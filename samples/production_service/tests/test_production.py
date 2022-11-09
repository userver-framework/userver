async def test_monitor(monitor_client):
    response = await monitor_client.get('/service/log-level/')
    assert response.status == 200


async def test_metrics_portability(service_client):
    warnings = await service_client.metrics_portability()
    assert not warnings, warnings
