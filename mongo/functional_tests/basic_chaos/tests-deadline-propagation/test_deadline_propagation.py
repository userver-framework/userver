import pytest

DP_TIMEOUT_MS = 'X-YaTaxi-Client-TimeoutMs'


@pytest.fixture(name='put_foo_value')
async def _put_foo_value(service_client):
    response = await service_client.get('/v1/key-value?key=foo')
    if response.status_code == 200:
        return
    assert response.status_code == 404
    response = await service_client.put('/v1/key-value?key=foo&value=bar')
    assert response.status_code == 200


async def test_works(service_client, put_foo_value, dynamic_config):
    assert dynamic_config.get('MONGO_DEADLINE_PROPAGATION_ENABLED_V2')

    async with service_client.capture_logs() as capture:
        response = await service_client.get(
            '/v1/key-value?key=foo&sleep_ms=200',
            headers={DP_TIMEOUT_MS: '100'},
        )
        assert response.status == 498
        assert response.text == 'Deadline expired'

    logs = [
        log
        for log in capture.select()
        if log['text'].startswith('exception in \'handler-key-value\'')
    ]
    assert len(logs) == 1
    text = logs[0]['text']
    assert 'Operation cancelled: deadline propagation' in text, text
    assert 'storages::mongo::CancelledException' in text, text


@pytest.mark.config(MONGO_DEADLINE_PROPAGATION_ENABLED_V2=False)
async def test_disabled(service_client, put_foo_value, dynamic_config):
    async with service_client.capture_logs() as capture:
        response = await service_client.get(
            '/v1/key-value?key=foo&sleep_ms=200',
            headers={DP_TIMEOUT_MS: '100'},
        )
        assert response.status == 498
        assert response.text == 'Deadline expired'

    # The mongo request should be completed ignoring the fact that
    # deadline has already expired.
    logs = capture.select(_type='response', meta_type='/v1/key-value')
    assert len(logs) == 1, logs
    assert logs[0].get('dp_original_body', None) == 'bar', str(logs)
