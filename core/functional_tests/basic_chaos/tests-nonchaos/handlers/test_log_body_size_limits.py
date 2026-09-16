HANDLER_PATH = '/chaos/httpserver'
STATIC_BODY_LOG_LIMIT = 512
DYNAMIC_REQUEST_BODY_LOG_LIMIT = 2
DYNAMIC_RESPONSE_BODY_LOG_LIMIT = 3
BODY = 'xy' * STATIC_BODY_LOG_LIMIT
INVALID_PATH = '/nonexistent/handler/path'


def _logged_prefix(log):
    return log['body'].partition('...(truncated,')[0]


async def _send_request_and_capture_logs(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.post(
            HANDLER_PATH,
            params={'type': 'echo'},
            data=BODY,
        )

    assert response.status_code == 200
    assert response.text == BODY

    request_logs = capture.select(
        _type='request',
        meta_type=HANDLER_PATH,
        level='INFO',
    )
    response_logs = capture.select(
        _type='response',
        meta_type=HANDLER_PATH,
        level='INFO',
    )

    assert len(request_logs) == 1
    assert len(response_logs) == 1
    return request_logs[0], response_logs[0]


async def test_dynamic_body_log_limits(service_client, dynamic_config):
    request_log, response_log = await _send_request_and_capture_logs(service_client)
    assert _logged_prefix(request_log) == BODY[:STATIC_BODY_LOG_LIMIT]
    assert _logged_prefix(response_log) == BODY[:STATIC_BODY_LOG_LIMIT]

    dynamic_config.set(
        USERVER_HTTP_SERVER_LOGS={
            HANDLER_PATH: {
                'request_body_size_log_limit': DYNAMIC_REQUEST_BODY_LOG_LIMIT,
                'response_body_size_log_limit': DYNAMIC_RESPONSE_BODY_LOG_LIMIT,
            },
        },
    )
    await service_client.update_server_state()

    request_log, response_log = await _send_request_and_capture_logs(service_client)
    assert _logged_prefix(request_log) == BODY[:DYNAMIC_REQUEST_BODY_LOG_LIMIT]
    assert _logged_prefix(response_log) == BODY[:DYNAMIC_RESPONSE_BODY_LOG_LIMIT]

    # response_body_size_log_limit is missing => value from a static config.
    dynamic_config.set(
        USERVER_HTTP_SERVER_LOGS={
            HANDLER_PATH: {
                'request_body_size_log_limit': DYNAMIC_REQUEST_BODY_LOG_LIMIT,
            },
        },
    )
    await service_client.update_server_state()

    request_log, response_log = await _send_request_and_capture_logs(service_client)
    assert _logged_prefix(request_log) == BODY[:DYNAMIC_REQUEST_BODY_LOG_LIMIT]
    assert _logged_prefix(response_log) == BODY[:STATIC_BODY_LOG_LIMIT]


async def test_warn_on_invalid_request_path(service_client, monitor_client, dynamic_config):
    async with service_client.capture_logs() as capture:
        dynamic_config.set(
            USERVER_HTTP_SERVER_LOGS={
                INVALID_PATH: {
                    'request_body_size_log_limit': 128,
                    'response_body_size_log_limit': 256,
                },
            },
        )
        await service_client.update_server_state()

    assert (await monitor_client.single_metric('alerts.invalid_request_path')).value == 1

    warning_logs = capture.select(
        level='WARNING',
        text=f"Typo in USERVER_HTTP_SERVER_LOGS: there aren't handlers in a server: [{INVALID_PATH}].",
    )
    assert len(warning_logs) >= 1, (
        f'Expected a WARNING about unknown path {INVALID_PATH!r} in USERVER_HTTP_SERVER_LOGS, but none was found'
    )
    dynamic_config.set(
        USERVER_HTTP_SERVER_LOGS={
            HANDLER_PATH: {
                'request_body_size_log_limit': 128,
                'response_body_size_log_limit': 256,
            },
        },
    )

    await service_client.update_server_state()

    assert (await monitor_client.single_metric('alerts.invalid_request_path')).value == 0


async def test_no_warn_on_valid_request_path(service_client, monitor_client, dynamic_config):
    async with service_client.capture_logs() as capture:
        dynamic_config.set(
            USERVER_HTTP_SERVER_LOGS={
                HANDLER_PATH: {
                    'request_body_size_log_limit': 128,
                    'response_body_size_log_limit': 256,
                },
            },
        )
        await service_client.update_server_state()

    assert (await monitor_client.single_metric('alerts.invalid_request_path')).value == 0

    warning_logs = capture.select(
        level='WARNING',
        text='Typo in USERVER_HTTP_SERVER_LOGS',
    )
    assert len(warning_logs) == 0, f'Unexpected WARNING about USERVER_HTTP_SERVER_LOGS for a valid path: {warning_logs}'
