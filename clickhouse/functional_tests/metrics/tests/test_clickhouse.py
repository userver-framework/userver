import re


def _normalize_metrics(metrics: str) -> str:
    result = []
    for line in metrics.splitlines():
        line = line.strip()
        if line.startswith('#') or not line:
            continue

        left, _ = line.rsplit('\t', 1)
        result.append(
            re.sub('localhost_\\d+', 'localhost_00000', left + '\t' + '0'),
        )
    result.sort()
    return '\n'.join(result)


async def test_metrics_smoke(monitor_client):
    metrics = await monitor_client.metrics(prefix='clickhouse.')
    assert len(metrics) > 1


async def test_metrics_portability(service_client):
    warnings = await service_client.metrics_portability(prefix='clickhouse.')
    assert not warnings


async def test_metrics(service_client, monitor_client, load):
    # Forcing statement timing metrics to appear
    response = await service_client.get('/metrics/clickhouse?key=test_key')
    assert response.status == 200
    assert response.content == b''

    response = await service_client.post(
        '/metrics/clickhouse?key=test_key&value=test_value',
    )
    assert response.status == 200
    assert response.content == b'test_key: test_value'

    response = await service_client.get('/metrics/clickhouse?key=test_key')
    assert response.status == 200
    assert response.content == b'test_key: test_value'

    response = await service_client.delete('/metrics/clickhouse?key=test_key')
    assert response.status == 200
    assert response.content == b'Deleted by key: test_key'

    ethalon = _normalize_metrics(load('metrics_values.txt'))
    assert ethalon
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(
            output_format='pretty', prefix='clickhouse.',
        ),
    )
    assert all_metrics == ethalon
