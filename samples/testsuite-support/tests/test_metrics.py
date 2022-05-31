from testsuite.utils import matching


async def test_basic(service_client, monitor_client):
    response = await service_client.get('/metrics')
    assert response.status_code == 200

    metrics = await monitor_client.get_metrics()
    assert metrics['sample-metrics']['foo'] > 0


# /// [metrics reset]
async def test_reset(service_client, monitor_client):
    # Reset service metrics
    await service_client.reset_metrics()
    # Retrieve metrics
    metrics = await monitor_client.get_metrics()
    assert metrics['sample-metrics']['foo'] == 0
    # /// [metrics reset]

    response = await service_client.get('/metrics')
    assert response.status_code == 200

    metrics = await monitor_client.get_metrics()
    assert metrics['sample-metrics']['foo'] == 1

    await service_client.reset_metrics()
    metrics = await monitor_client.get_metrics()
    assert metrics['sample-metrics']['foo'] == 0


async def test_specific(service_client, monitor_client):
    await service_client.reset_metrics()

    response = await service_client.get('/metrics')
    assert response.status_code == 200

    metrics = await monitor_client.get_metrics('sample-metrics')
    assert metrics == {
        '$version': matching.non_negative_integer,
        'sample-metrics': {'foo': 1},
    }
