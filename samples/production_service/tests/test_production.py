async def test_monitor(monitor_client):
    response = await monitor_client.get('/service/log-level/')
    assert response.status == 200


async def test_metrics_smoke(monitor_client):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1


# /// [metrics partial portability]
async def test_partial_metrics_portability(service_client):
    warnings = await service_client.metrics_portability()
    warnings.pop('label_name_mismatch', None)
    assert not warnings, warnings
    # /// [metrics partial portability]
