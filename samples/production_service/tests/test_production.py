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


async def test_jemalloc_handle(monitor_client):
    response = await monitor_client.post('service/jemalloc/prof/disable')
    if response.status_code == 501:
        # No jemalloc support
        return
    else:
        # Handle always succeeds even if was not previously enabled
        assert response.status_code == 200

    response = await monitor_client.post('service/jemalloc/prof/stat')
    assert response.status_code == 200
    assert response.text

    try:
        response = await monitor_client.post('service/jemalloc/prof/enable')
        assert response.status_code == 503
        assert 'MALLOC_CONF' in response.text
        assert 'prof:true' in response.text
    finally:
        response = await monitor_client.post('service/jemalloc/prof/disable')
        assert response.status_code == 200
