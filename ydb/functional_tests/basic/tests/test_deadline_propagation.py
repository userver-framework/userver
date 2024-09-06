import pytest


DP_TIMEOUT_MS = 'X-YaTaxi-Client-TimeoutMs'
TIMEOUT = '100000'

JSON = {
    'id': 'id-upsert',
    'name': 'name-upsert',
    'service': 'srv',
    'channel': 123,
}

QUERY_NAMES = ('Begin', 'upsert-row', 'Commit')


def assert_deadline_timeout(
        capture, *, query_names=QUERY_NAMES, expect_dp_enabled: bool = True,
):
    for query in query_names:
        logs = capture.select(stopwatch_name='ydb_query', query_name=query)
        assert len(logs) == 1, str(logs)
        if expect_dp_enabled:
            assert 'deadline_timeout_ms' in logs[0]
        else:
            assert 'deadline_timeout_ms' not in logs[0]


async def test_on(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            'ydb/upsert-row', headers={DP_TIMEOUT_MS: TIMEOUT}, json=JSON,
        )
        assert response.status_code == 200

    assert_deadline_timeout(capture)


async def test_triggered(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            'ydb/upsert-row', headers={DP_TIMEOUT_MS: '5'}, json=JSON,
        )
        assert response.status_code == 498

    assert_deadline_timeout(capture, query_names=['Begin'])


@pytest.mark.config(YDB_DEADLINE_PROPAGATION_VERSION=0)
async def test_config_disabled(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            'ydb/upsert-row', headers={DP_TIMEOUT_MS: TIMEOUT}, json=JSON,
        )
        assert response.status_code == 200

    assert_deadline_timeout(capture, expect_dp_enabled=False)


async def test_off(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post('ydb/upsert-row', json=JSON)
        assert response.status_code == 200

    assert_deadline_timeout(capture, expect_dp_enabled=False)
