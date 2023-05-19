def _normalize_metrics(metrics: str) -> str:
    result = []
    for line in metrics.splitlines():
        line = line.strip()
        if line.startswith('#') or not line:
            continue

        left, _ = line.rsplit('\t', 1)
        result.append(left + '\t' + '0')
    return '\n'.join(result) + '\n'


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

    ethalon = load('metrics_values.txt')
    all_metrics = _normalize_metrics(
        await monitor_client.metrics_raw(
            output_format='pretty', prefix='rabbitmq',
        ),
    )
    assert all_metrics == ethalon
