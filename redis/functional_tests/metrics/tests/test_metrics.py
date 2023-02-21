import re
import typing


SERVICE_COMMANDS = ('sentinel', 'cluster', 'info', 'ping')


def _normalize_metrics(metrics: str) -> typing.Set[str]:
    result = set()
    for line in metrics.splitlines():
        # skip metrics unrelated to redis
        if 'redis' not in line:
            continue

        # skip metrics related to service commands
        # since we cannot force them to appear
        if any(('redis_command=' + cmd) in line for cmd in SERVICE_COMMANDS):
            continue

        # TODO: make the metrics less flapping:
        if 'redis_instance_type=slaves' in line:
            continue

        left, _, _ = line.rsplit(' ', 2)

        left = re.sub('localhost_\\d+', 'localhost_00000', left)
        left = re.sub(
            'redis_instance=[\\d_a-z.]+',
            'redis_instance=localhost_00001',
            left,
        )
        result.add(left + ' ' + '0')

    return result


async def test_metrics_smoke(service_client, monitor_client):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1


async def test_metrics_portability(service_client):
    warnings = await service_client.metrics_portability()
    warnings.pop('label_name_mismatch')
    assert not warnings


async def test_metrics(service_client, monitor_client, load, mocked_time):
    # Forcing redis metrics to appear
    response = await service_client.post('/v1/metrics?key=key_1')
    assert response.status == 201
    response = await service_client.get('/v1/metrics?key=key_1')
    assert response.status == 200
    response = await service_client.delete('/v1/metrics?key=key_1')
    assert response.status == 200

    mocked_time.sleep(10)
    await service_client.invalidate_caches()

    ethalon = _normalize_metrics(load('metrics_values.txt'))
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(output_format='graphite'),
    )
    assert all_metrics == ethalon
