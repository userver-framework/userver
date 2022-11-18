import sys


def _normalize_metrics(metrics: str) -> str:
    result = []
    for line in metrics.splitlines():
        left, _, _ = line.rsplit(' ', 2)

        if sys.platform == 'darwin' and left.startswith('io_'):
            # MacOS does not provide some of the io_* metrics
            continue

        if ';' in left:
            path, labels = left.split(';', 1)
            label_keys = [x.split('=', 1)[0] + '=' for x in labels.split(';')]
            left = path + ';' + ';'.join(label_keys)

        result.append(left + ' ' + '0')

    # Leave only unique values to deal with httpclient metric flaps
    # TODO: remove
    result = list(set(result))

    result.sort()
    return '\n'.join(result)


async def test_metrics_smoke(monitor_client):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1


async def test_metrics(monitor_client, load):
    ethalon = _normalize_metrics(load('metrics_values.txt'))
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(output_format='graphite'),
    )
    assert all_metrics == ethalon
