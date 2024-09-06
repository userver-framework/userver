async def test_metrics_smoke(monitor_client, force_metrics_to_appear):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1


async def test_metrics_portability(service_client, force_metrics_to_appear):
    warnings = await service_client.metrics_portability()
    assert not warnings


def _is_mongo_metric(line: str) -> bool:
    line = line.strip()
    if line.startswith('#') or not line:
        return False

    if 'mongo' not in line and 'distlock' not in line:
        return False

    # These errors sometimes appear during service startup,
    # it's tedious to reproduce them for metrics tests.
    if 'mongo.pool.conn-init.errors' in line and (
            'mongo_error=network' in line
            or 'mongo_error=cluster-unavailable' in line
    ):
        return False

    return True


def _normalize_metrics(metrics: str) -> str:
    result = [line for line in metrics.splitlines() if _is_mongo_metric(line)]
    result.sort()
    return '\n'.join(result)


def _hide_metrics_values(metrics: str) -> str:
    return '\n'.join(line.rsplit('\t', 1)[0] for line in metrics.splitlines())


async def test_metrics(monitor_client, load, force_metrics_to_appear):
    ground_truth = _normalize_metrics(load('metrics_values.txt'))
    assert ground_truth
    all_metrics = await monitor_client.metrics_raw(output_format='pretty')
    all_metrics = _normalize_metrics(all_metrics)
    all_metrics_paths = _hide_metrics_values(all_metrics)
    ground_truth_paths = _hide_metrics_values(ground_truth)

    assert all_metrics_paths == ground_truth_paths, (
        '\n===== Service metrics start =====\n'
        f'{all_metrics}\n'
        '===== Service metrics end =====\n'
    )
