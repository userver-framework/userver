async def test_expired(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            '/chaos/postgres?sleep_ms=1000&type=select',
            headers={'X-YaTaxi-Client-TimeoutMs': '500'},
        )
        assert response.status == 498
        assert response.text == 'Deadline expired'

    logs = [log for log in capture.select() if log['text'].startswith("exception in 'handler-chaos-postgres'")]
    assert len(logs) == 1
    text = logs[0]['text']
    assert 'storages::postgres::ConnectionInterrupted' in text, text


async def test_timeout(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            '/chaos/postgres?type=sleep',
            headers={'X-YaTaxi-Client-TimeoutMs': '150'},
        )
        assert response.status == 498
        assert response.text == 'Deadline expired'

    logs = [log for log in capture.select() if log['text'].startswith('Statement')]
    assert len(logs) == 1
    text = logs[0]['text']
    assert 'was cancelled by deadline propagation' in text, text
