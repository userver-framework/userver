import typing


def _normalize_metrics(metrics: str) -> str:
    result: typing.List[str] = []
    for line in metrics.splitlines():
        if 'rabbitmq' in line:
            left, _, _ = line.rsplit(' ', 2)
            result.append(left + ' ' + '0')

    result.sort()
    return '\n'.join(result)


async def test_metrics_smoke(monitor_client):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1


async def test_metrics_portability(service_client):
    warnings = await service_client.metrics_portability()
    assert not warnings


async def test_metrics(service_client, monitor_client, testpoint, load):
    # Forcing metrics to appear
    @testpoint('message_consumed')
    def message_consumed(data):
        pass

    num_messages = 10

    for i in range(num_messages):
        response = await service_client.post(
            '/v1/messages/', json={'message': str(i)},
        )
        assert response.status_code == 200

    for i in range(num_messages):
        await message_consumed.wait_call()

    response = await service_client.get('/v1/messages')
    assert response.status_code == 200

    ethalon = _normalize_metrics(load('metrics_values.txt'))
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(output_format='graphite'),
    )
    assert all_metrics == ethalon
