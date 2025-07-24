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
        (
            {'long_header': 'A' * 1000},  # default limit is 512
            ['long_header'],
            {'HEADERS-DID-NOT-FIT-IN-SIZE-LIMIT': True},
        ),
    ],
)
async def test_log_request_headers(
    service_client,
    dynamic_config,
    headers,
    headers_whitelist,
    expected_logged_headers,
):
    # /// [dynamic_config usage]
    dynamic_config.set(
        USERVER_LOG_REQUEST_HEADERS=True,
        USERVER_LOG_REQUEST_HEADERS_WHITELIST=headers_whitelist,
    )
    await service_client.update_server_state()
    # /// [dynamic_config usage]

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
        if header_name.lower() not in DEFAULT_TESTSUITE_HEADERS
    }

    assert request_headers == expected_logged_headers


def _contains_ordered_substrings(string, expected_ordered_substrings):
    current_index = 0
    for substring in expected_ordered_substrings:
        current_index = string.find(substring, current_index)
        if current_index == -1:
            return False
        current_index += len(substring)
    return True


@pytest.mark.parametrize(
    'headers, headers_whitelist, expected_ordered_substrings',
    [
        (  #
            {'d_header': 'd_value', 'c_header': 'c_value', 'a_header': 'a_value', 'b_header': 'b_value'},
            ['a_header', 'c_header'],
            # Whitelisted headers go first (sorted by header name), "secret" headers go second (sorted by header name).
            ['a_header', 'a_value', 'c_header', 'c_value', 'b_header', '***', 'd_header', '***'],
        ),
    ],
)
async def test_order(
    service_client,
    dynamic_config,
    headers,
    headers_whitelist,
    expected_ordered_substrings,
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

    assert _contains_ordered_substrings(request_headers_raw, expected_ordered_substrings)
