import json

import pytest

DEFAULT_TESTSUITE_HEADERS = frozenset([
    'user-agent',
    'accept-encoding',
    'accept',
    'x-yaspanid',
    'x-yatraceid',
    'host',
])


@pytest.mark.parametrize(
    'headers, headers_whitelist, expected_logged_headers',
    [
        (  #
            {'secret_header': 'secret'},
            [],
            {'secret_header': '***'},
        ),
        (
            {'legal_header': 'userver best framework'},
            ['legal_header'],
            {'legal_header': 'userver best framework'},
        ),
    ],
)
async def get_log_request_headers(
    service_client,
    dynamic_config,
    headers,
    headers_whitelist,
    expected_logged_headers,
):
    dynamic_config.set(
        USERVER_LOG_REQUEST_HEADERS=True,
        USERVER_LOG_REQUEST_HEADERS_WHITELIST=headers_whitelist,
    )
    await service_client.update_server_state()

    async with service_client.capture_logs() as capture:
        await service_client.get(
            '/chaos/httpserver',
            params={'type': 'echo'},
            headers=headers,
        )

    logs = capture.select(
        _type='request',
        meta_type='/chaos/httpserver',
        level='INFO',
    )

    assert len(logs) == 1

    request_headers_raw = logs[0]['request_headers']

    request_headers = json.loads(request_headers_raw)

    request_headers = {
        header_name: header_value
        for header_name, header_value in request_headers.items()
        if header_name.lowercase() not in DEFAULT_TESTSUITE_HEADERS
    }

    assert request_headers == expected_logged_headers
