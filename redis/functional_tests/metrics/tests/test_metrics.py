import re


SERVICE_COMMANDS = ('sentinel', 'cluster', 'info', 'ping')


def _is_redis_metrics(line: str) -> bool:
    # skip metrics related to service commands
    # since we cannot force them to appear
    if (
            'redis' not in line
            or any(
                ('redis_command=' + cmd) in line for cmd in SERVICE_COMMANDS
            )
            or 'redis_instance_type=slaves' in line
    ):
        return False
    return True


def _normalize_metrics(metrics: str) -> str:
    result = []
    for line in metrics.splitlines():
        line = line.strip()
        if line.startswith('#') or not line:
            continue

        if not _is_redis_metrics(line):
            continue

        left, _ = line.rsplit('\t', 1)

        left = re.sub('localhost:\\d+', '127.0.0.1:00000', left + '\t' + '0')
        left = re.sub('127.0.0.1:\\d+', '127.0.0.1:00000', left)
        left = re.sub('::1:\\d+', '127.0.0.1:00000', left)
        left = re.sub(
            '[a-f0-9]*:[a-f0-9]*:[a-f0-9]*:[a-f0-9]*:[a-f0-9]*:[a-f0-9]*',
            '127.0.0.1:00000',
            left,
        )
        result.append(left)
    result.sort()
    return '\n'.join(result) + '\n'


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
    assert ethalon
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(output_format='pretty'),
    )
    assert all_metrics == ethalon
