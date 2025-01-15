import pytest


async def test_expired(service_client, dynamic_config):
    assert dynamic_config.get('POSTGRES_DEADLINE_PROPAGATION_VERSION') == 1

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


@pytest.mark.config(POSTGRES_DEADLINE_PROPAGATION_VERSION=0)
async def test_expired_dp_disabled(service_client, dynamic_config):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            '/chaos/postgres?sleep_ms=1000&type=select',
            headers={'X-YaTaxi-Client-TimeoutMs': '500'},
        )
        assert response.status == 498
        assert response.text == 'Deadline expired'

    pipeline_enabled = dynamic_config.get(
        'POSTGRES_CONNECTION_PIPELINE_EXPERIMENT',
    )

    logs = capture.select(_type='response', meta_type='/chaos/postgres')
    assert len(logs) == 1, logs
    if pipeline_enabled:
        assert not logs[0].get('dp_original_body', None), logs
    else:
        assert logs[0].get('dp_original_body', None), logs
