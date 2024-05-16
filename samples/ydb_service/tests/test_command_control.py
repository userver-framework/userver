import pytest


async def test_query_span(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            'ydb/select-rows',
            json={
                'service': 'srv',
                'channels': [1, 2, 3],
                'created': '2019-10-30T11:20:00+00:00',
            },
        )
        assert response.status_code == 200
        assert response.json() == {'items': []}

    assert capture.select(
        link=response.headers['x-yarequestid'],
        stopwatch_name='ydb_query',
        max_retries='3',
        get_session_timeout_ms='5000',
        operation_timeout_ms='1000',
        cancel_after_ms='1000',
        client_timeout_ms='1100',
    )


@pytest.mark.config(
    YDB_QUERIES_COMMAND_CONTROL={
        'select': {
            'attempts': 10,
            'operation-timeout-ms': 5001,
            'cancel-after-ms': 5002,
            'client-timeout-ms': 5003,
            'get-session-timeout-ms': 5004,
        },
    },
)
async def test_config_command_control(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            'ydb/select-rows',
            json={
                'service': 'srv',
                'channels': [1, 2, 3],
                'created': '2019-10-30T11:20:00+00:00',
            },
        )
        assert response.status_code == 200
        assert response.json() == {'items': []}

    assert capture.select(
        link=response.headers['x-yarequestid'],
        stopwatch_name='ydb_query',
        max_retries='10',
        get_session_timeout_ms='5004',
        operation_timeout_ms='5001',
        cancel_after_ms='5002',
        client_timeout_ms='5003',
    )
