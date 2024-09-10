import json

default_testsuite_headers = [
    'User-Agent',
    'Accept-Encoding',
    'Accept',
    'X-YaSpanId',
    'Host',
]


async def get_log_requst_headers(
    service_client, dynamic_config, headers, headers_whitelist,
):
    dynamic_config.set(
        USERVER_LOG_REQUEST_HEADERS=True,
        USERVER_LOG_REQUEST_HEADERS_WHITELIST=headers_whitelist,
    )
    await service_client.update_server_state()

    async with service_client.capture_logs() as capture:
        await service_client.get(
            '/chaos/httpserver', params={'type': 'echo'}, headers=headers,
        )

    logs = capture.select(
        _type='request', meta_type='/chaos/httpserver', level='INFO',
    )

    assert len(logs) == 1

    request_headers_raw = logs[0]['request_headers']

    request_headers = json.loads(request_headers_raw)

    for header_name in default_testsuite_headers:
        del request_headers[header_name]
    return request_headers


async def test_headers(service_client, dynamic_config):
    assert await get_log_requst_headers(
        service_client,
        dynamic_config,
        headers={'secret_header': 'secret'},
        headers_whitelist=[],
    ) == {'secret_header': '***'}
    assert await get_log_requst_headers(
        service_client,
        dynamic_config,
        headers={'legal_header': 'userver best framework'},
        headers_whitelist=['legal_header'],
    ) == {'legal_header': 'userver best framework'}
