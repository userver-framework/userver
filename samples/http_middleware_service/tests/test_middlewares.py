async def test_server_middleware(service_client):
    response = await service_client.get('/hello')
    assert response.status == 200
    assert response.content == b'Hello world!\n'
    # this is set by server-wide middleware we added
    assert 'X-Some-Server-Header' in response.headers.keys()
    assert 'X-Some-Handler-Header' not in response.headers.keys()


async def _check_handler_with_custom_middleware(
        service_client, uri, expected_header_value,
):
    response = await service_client.get(uri)
    assert response.status == 200
    assert response.content == b'Hello world!\n'
    assert 'X-Some-Server-Header' in response.headers.keys()
    # our per-handler middleware pipeline builder should
    # add the middleware that sets this header
    assert 'X-Some-Handler-Header' in response.headers.keys()
    assert response.headers['X-Some-Handler-Header'] == expected_header_value


async def test_custom_handler_middleware(service_client):
    await _check_handler_with_custom_middleware(
        service_client, '/custom-hello', 'some_value',
    )

    await _check_handler_with_custom_middleware(
        service_client, '/custom-hello-other', 'some_other_value',
    )
