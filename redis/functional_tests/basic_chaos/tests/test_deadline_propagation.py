import pytest

DP_TIMEOUT_MS = 'X-YaTaxi-Client-TimeoutMs'


async def test_expired(service_client, dynamic_config, sentinel_gate, gate):
    assert dynamic_config.get('REDIS_DEADLINE_PROPAGATION_VERSION') == 1

    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            '/chaos?key=foo&value=bar&sleep_ms=1000',
            headers={DP_TIMEOUT_MS: '500'},
        )
        assert response.status == 498
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


@pytest.mark.config(REDIS_DEADLINE_PROPAGATION_VERSION=0)
async def test_expired_dp_disabled(service_client, sentinel_gate, gate):
    response = await service_client.post('/chaos?key=foo&value=bar')
    assert response.status == 201

    async with service_client.capture_logs() as capture:
        response = await service_client.get(
            '/chaos?key=foo&sleep_ms=1000', headers={DP_TIMEOUT_MS: '500'},
        )
        assert response.status == 498
        assert response.text == 'Deadline expired'

    # The redis request should be completed ignoring the fact that
    # deadline has already expired.
    logs = capture.select(_type='response', meta_type='/chaos')
    assert len(logs) == 1, logs
    assert logs[0].get('dp_original_body', None) == 'bar', str(logs)
