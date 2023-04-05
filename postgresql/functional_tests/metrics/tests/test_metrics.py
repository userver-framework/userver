import re


def _normalize_metrics(metrics: str) -> str:
    result = []
    for line in metrics.splitlines():
        if (
                'key-value-pg-cache' not in line
                and 'postgre' not in line
                and 'distlock' not in line
        ):
            continue
        left, _, _ = line.rsplit(' ', 2)

        left = re.sub('localhost_\\d+', 'localhost_00000', left)
        result.append(left + ' ' + '0')

    result.sort()
    return '\n'.join(result)


async def test_metrics_smoke(monitor_client):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1


async def test_metrics_portability(service_client):
    warnings = await service_client.metrics_portability()
    assert not warnings


async def test_metrics(service_client, monitor_client, load):
    # Forcing statement timing metrics to appear
    response = await service_client.get('/metrics/postgres?type=fill')
    assert response.status == 200

    # Make a portal portal to capture its metrics (if any)
    response = await service_client.get('/metrics/postgres?type=portal')
    assert response.status == 200

    ethalon = _normalize_metrics(load('metrics_values.txt'))
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(output_format='graphite'),
    )
    assert all_metrics == ethalon
