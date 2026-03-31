import pytest

DP_TIMEOUT_MS = 'X-YaTaxi-Client-TimeoutMs'

REDIS_TOPOLOGY_CASES = [
    ('/redis-cluster', 'handler-cluster'),
    ('/redis-sentinel', 'handler-sentinel'),
    ('/redis-standalone', 'handler-standalone'),
]


@pytest.mark.parametrize('redis_http_path,handler_component_name', REDIS_TOPOLOGY_CASES)
async def test_deadline_propagation_expired(
    service_client,
    dynamic_config,
    redis_http_path,
    handler_component_name,
):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            redis_http_path,
            params={'key': 'dp_blpop_deadline_list', 'blpop_wait_sec': '120'},
            headers={DP_TIMEOUT_MS: '500'},
        )
        assert response.status == 498
        assert response.text == 'Deadline expired'

    prefix = f"exception in '{handler_component_name}'"
    logs = [log for log in capture.select() if log['text'].startswith(prefix)]
    assert len(logs) == 1
    text = logs[0]['text']
    assert "request failed with status 'timeout'" in text, text
    assert 'redis::RequestFailedException' in text, text
