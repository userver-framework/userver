import pytest


@pytest.mark.config(POSTGRES_DEADLINE_PROPAGATION_VERSION=1)
async def test_expired(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            '/chaos/postgres?sleep_ms=1000&type=select',
            headers={'X-YaTaxi-Client-TimeoutMs': '500'},
        )
        assert response.status == 504
        assert response.text == 'Deadline expired'

    logs = [
        log
        for log in capture.select()
        if log['text'].startswith('exception in \'handler-chaos-postgres\'')
    ]
    assert len(logs) == 1
    assert (
        'storages::postgres::ConnectionInterrupted' in logs[0]['text']
    ), logs[0]['text']
