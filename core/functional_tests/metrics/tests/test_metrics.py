import re
import sys


def _normalize_metrics(metrics: str) -> str:
    result = []
    for line in metrics.splitlines():
        line = line.strip()
        if line.startswith('#') or not line:
            continue

        left, _ = line.rsplit('\t', 1)
        if sys.platform == 'darwin' and left.startswith('io_'):
            # MacOS does not provide some of the io_* metrics
            continue
        left = re.sub('localhost:\\d+', 'localhost:00000', left + '\t' + '0')
        result.append(left)
    result.sort()
    return '\n'.join(result) + '\n'


async def test_metrics_smoke(monitor_client):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1

    cache_hits = await monitor_client.single_metric('cache.hits')
    assert cache_hits.value >= 0

    cache_hits = await monitor_client.metrics(prefix='cache.hits')
    assert cache_hits.value_at('cache.hits') >= 0


async def test_metrics(monitor_client, load):
    ethalon = _normalize_metrics(load('metrics_values.txt'))
    assert ethalon
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(output_format='pretty'),
    )
    assert all_metrics == ethalon
