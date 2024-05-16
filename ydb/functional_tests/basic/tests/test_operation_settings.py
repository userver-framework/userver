async def test_get_settings_from_user_code(service_client):
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
        get_session_timeout_ms='4999',
        operation_timeout_ms='1000',
        cancel_after_ms='1000',
        client_timeout_ms='1100',
    )


async def test_get_settings_from_static_config(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            'ydb/upsert-row',
            json={
                'id': 'id-upsert',
                'name': 'name-upsert',
                'service': 'srv',
                'channel': 123,
            },
        )
        assert response.status_code == 200
        assert response.json() == {}

    assert capture.select(
        link=response.headers['x-yarequestid'],
        stopwatch_name='ydb_query',
        max_retries='2',
        get_session_timeout_ms='5001',
        operation_timeout_ms='1001',
        cancel_after_ms='1001',
        client_timeout_ms='1101',
    )


async def test_get_settings_from_dynamic_config(
        service_client, dynamic_config,
):
    async with service_client.capture_logs() as capture:
        operation_settings = {
            'select': {
                'attempts': 4,
                'operation-timeout-ms': 1002,
                'cancel-after-ms': 1002,
                'client-timeout-ms': 1102,
                'get-session-timeout-ms': 5002,
            },
        }
        dynamic_config.set_values(
            {'YDB_QUERIES_COMMAND_CONTROL': operation_settings},
        )
        await service_client.update_server_state()

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
        max_retries='4',
        operation_timeout_ms='1002',
        cancel_after_ms='1002',
        client_timeout_ms='1102',
        get_session_timeout_ms='5002',
    )
