import re
import sys


def _normalize_metrics(metrics: str) -> str:
    result = []
    for line in metrics.splitlines():
        left, _, _ = line.rsplit(' ', 2)

        if sys.platform == 'darwin' and left.startswith('io_'):
            # MacOS does not provide some of the io_* metrics
            continue

        left = re.sub('localhost_\\d+', 'localhost_00000', left)
        result.append(left + ' ' + '0')

    result.sort()
    return '\n'.join(result)


async def test_metrics_smoke(monitor_client):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1

    cache_hits = await monitor_client.single_metric('cache.hits')
    assert cache_hits.value >= 0

    cache_hits = await monitor_client.metrics(prefix='cache.hits')
    assert cache_hits.value_at('cache.hits') >= 0


async def test_metrics(monitor_client, load):
    ethalon = _normalize_metrics(load('metrics_values.txt'))
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(output_format='graphite'),
    )
    assert all_metrics == ethalon
