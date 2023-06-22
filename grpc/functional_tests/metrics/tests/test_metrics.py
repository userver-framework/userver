import typing


def _normalize_metrics(metrics: str) -> typing.Set[str]:
    result = metrics.splitlines()

    result = _drop_non_grpc_metrics(result)
    result = _hide_metrics_values(result)

    return set(result)


def _drop_non_grpc_metrics(metrics: typing.List[str]) -> typing.List[str]:
    result = []
    for line in metrics:
        if line.startswith(('grpc.server', 'grpc.client')):
            result.append(line)

    return result


def _hide_metrics_values(metrics: typing.List[str]) -> typing.List[str]:
    return [line.rsplit('\t', 1)[0] for line in metrics]


async def test_metrics_smoke(monitor_client):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1

    cache_hits = await monitor_client.single_metric(
        'grpc.client.by-destination.deadline-propagated',
    )
    assert cache_hits.value >= 0

    cache_hits = await monitor_client.metrics(
        prefix='grpc.client.by-destination.deadline-propagated',
    )
    assert (
        cache_hits.value_at('grpc.client.by-destination.deadline-propagated')
        >= 0
    )


async def test_metrics(monitor_client, load):
    reference = _normalize_metrics(load('metrics_values.txt'))
    assert reference
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(output_format='pretty'),
    )
    assert all_metrics == reference
    assert all_metrics == reference, (
        '\n===== Service metrics start =====\n'
        f'{all_metrics}\n'
        '===== Service metrics end =====\n'
    )
