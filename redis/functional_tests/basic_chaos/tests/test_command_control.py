import math


def _is_redis_metrics(line: str) -> bool:
    # Get only redis_error=ok related metrics
    return (
        'redis_error=OK' in line
        and ('redis_instance_type=slaves' in line or 'redis_instance_type=masters' in line)
        and 'redis.errors.v2' in line
        and 'redis_instance=' in line
    )


def _normalize_metrics(metrics: str) -> str:
    result = []
    for line in metrics.splitlines():
        line = line.strip()
        if line.startswith('#') or not line:
            continue

        if not _is_redis_metrics(line):
            continue

        result.append(line)
    return result


async def test_redis_cc_consider_ping_false(service_client, sentinel_gate, gate, monitor_client):
    # make one of the connection be slower than others
    await gate.to_server_delay(0.1)
    await service_client.get('/chaos-many-requests?consider_ping=False')
    await gate.to_server_pass()

    all_metrics = _normalize_metrics(await monitor_client.metrics_raw(output_format='pretty'))
    assert len(all_metrics) == 3
    values = [float(line.split()[-1]) for line in all_metrics]
    assert math.isclose(values[0], values[1], abs_tol=20) and math.isclose(values[1], values[2], abs_tol=20)


async def test_redis_cc_consider_ping_true(service_client, sentinel_gate, gate, monitor_client):
    # make one of the connection be slower than others
    await gate.to_server_delay(0.1)
    await service_client.get('/chaos-many-requests?consider_ping=True')
    await gate.to_server_pass()

    all_metrics = _normalize_metrics(await monitor_client.metrics_raw(output_format='pretty'))
    assert len(all_metrics) == 3
    values = [float(line.split()[-1]) for line in all_metrics]
    assert not (math.isclose(values[0], values[1], abs_tol=20) and math.isclose(values[1], values[2], abs_tol=20))
