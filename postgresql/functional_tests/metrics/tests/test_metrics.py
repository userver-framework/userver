import re


def _is_postgresql_metrics(line: str) -> bool:
    if (
            'key-value-pg-cache' not in line
            and 'postgresql' not in line
            and 'distlock' not in line
    ):
        return False
    return True


def _normalize_metrics(metrics: str) -> str:
    result = []
    for line in metrics.splitlines():
        line = line.strip()
        if line.startswith('#') or not line:
            continue

        if not _is_postgresql_metrics(line):
            continue
        left, _ = line.rsplit('\t', 1)
        result.append(
            re.sub('localhost:\\d+', 'localhost:00000', left + '\t' + '0'),
        )
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
    assert ethalon
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(output_format='pretty'),
    )

    assert all_metrics == ethalon
