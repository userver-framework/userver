import pytest

DP_TIMEOUT_MS = 'X-YaTaxi-Client-TimeoutMs'
TIMEOUT = '100000'

JSON = {
    'id': 'id-upsert',
    'name': 'name-upsert',
    'service': 'srv',
    'channel': 123,
}

QUERY_NAMES = ('GetSession', 'Begin', 'upsert-row', 'Commit')
QUERY_NAMES_OLD = ('Begin', 'upsert-row', 'Commit')


def assert_deadline_timeout(
    capture,
    *,
    query_names,
    expect_dp_enabled: bool = True,
):
    for query in query_names:
        logs = capture.select(stopwatch_name='ydb_query', query_name=query)
        assert len(logs) == 1, str(logs)
        if expect_dp_enabled:
            assert 'deadline_timeout_ms' in logs[0]
        else:
            assert 'deadline_timeout_ms' not in logs[0]


def get_query_names(handler):
    return QUERY_NAMES if handler == 'upsert-row' else QUERY_NAMES_OLD


@pytest.mark.parametrize('handler', ['upsert-row', 'upsert-row-old'])
async def test_on(service_client, handler):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            f'ydb/{handler}',
            headers={DP_TIMEOUT_MS: TIMEOUT},
            json=JSON,
        )
        assert response.status_code == 200

    assert_deadline_timeout(capture, query_names=get_query_names(handler))


@pytest.mark.parametrize('handler', ['upsert-row-old'])
async def test_triggered_old(service_client, handler):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            f'ydb/{handler}',
            headers={DP_TIMEOUT_MS: '5'},
            json=JSON,
        )
        assert response.status_code == 498

    if handler == 'upsert-row-old':
        assert_deadline_timeout(capture, query_names=['Begin'])
        assert len(capture.select(stopwatch_name='ydb_query')) == 1
    else:
        assert len(capture.select(stopwatch_name='ydb_query')) == 0


@pytest.mark.config(YDB_DEADLINE_PROPAGATION_VERSION=0)
@pytest.mark.parametrize('handler', ['upsert-row', 'upsert-row-old'])
async def test_config_disabled(service_client, handler):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            f'ydb/{handler}',
            headers={DP_TIMEOUT_MS: TIMEOUT},
            json=JSON,
        )
        assert response.status_code == 200

    assert_deadline_timeout(capture, query_names=get_query_names(handler), expect_dp_enabled=False)


@pytest.mark.parametrize('handler', ['upsert-row', 'upsert-row-old'])
async def test_off(service_client, handler):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(f'ydb/{handler}', json=JSON)
        assert response.status_code == 200

    assert_deadline_timeout(capture, query_names=get_query_names(handler), expect_dp_enabled=False)
