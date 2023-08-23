async def test_expired(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            '/chaos?key=foo&value=bar&sleep_ms=1000',
            headers={'X-YaTaxi-Client-TimeoutMs': '500'},
        )
        assert response.status == 504
        assert response.text == 'Deadline expired'

    logs = [
        log
        for log in capture.select()
        if log['text'].startswith('exception in \'handler-chaos\'')
    ]
    assert len(logs) == 1
    text = logs[0]['text']
    assert 'request failed with status \'timeout\'' in text, text
    assert 'redis::RequestFailedException' in text, text
