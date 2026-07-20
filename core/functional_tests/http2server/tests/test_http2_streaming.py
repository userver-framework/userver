import asyncio

DEFAULT_PATH = '/http2server-stream'


async def test_body_stream(http2_client, service_client, dynamic_config):
    part = 'part'
    count = 100
    r = await http2_client.get(
        DEFAULT_PATH,
        params={'type': 'eq', 'body_part': part, 'count': count},
    )
    assert 200 == r.status_code
    assert part * count == r.text


async def _stream_request(client, req_per_client):
    for _ in range(req_per_client):
        data = 'abcdefgh' * 2**7  # size is 1024
        r = await client.get(DEFAULT_PATH, params={'type': 'ne'}, data=data)
        assert 200 == r.status_code
        assert data == r.text


async def test_body_stream_small_pieces(
    http2_client,
    service_client,
    dynamic_config,
):
    await _stream_request(http2_client, 1)


async def test_body_stream_concurrent(
    http2_client,
    service_client,
    dynamic_config,
):
    clients_count = 2
    req_per_client = 10
    tasks = [_stream_request(http2_client, req_per_client) for _ in range(clients_count)]
    await asyncio.gather(*tasks)


async def test_body_stream_no_head_of_line_blocking(
    http2_client,
    service_client,
    dynamic_config,
):
    # A slow streamed response (~5s) on one stream must not delay other
    # requests multiplexed on the same connection. If it did, each "fast"
    # request below would complete only after the slow stream finishes and
    # trip its timeout.
    part = 'x'
    count = 50
    slow = asyncio.create_task(
        http2_client.get(
            DEFAULT_PATH,
            params={
                'type': 'eq',
                'body_part': part,
                'count': count,
                'delay_ms': 100,
            },
            timeout=30.0,
        ),
    )
    try:
        for _ in range(5):
            r = await asyncio.wait_for(
                http2_client.get(
                    '/http2server',
                    params={'type': 'echo-body'},
                    data='ping',
                ),
                timeout=2.0,
            )
            assert 200 == r.status_code
            assert 'ping' == r.text
    finally:
        r = await slow
    assert 200 == r.status_code
    assert part * count == r.text
