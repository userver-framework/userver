async def call(service_client):
    async with service_client.capture_logs() as capture:
        response = await service_client.get('/hello', params={'name': 'john'})
        assert response.status == 200
        assert response.text == 'tost'

    return capture.select(
        stopwatch_name='GET /test',
    )[0]


async def test_hello_base(service_client, mockserver, dynamic_config):
    # /// [Functional test]
    @mockserver.handler('test/test')
    def test(request):
        assert request.query['name'] == 'john'
        return mockserver.make_response('"tost"')
        # /// [Functional test]

    p = await call(service_client)
    assert p['http.request.max_resend_count'] == '1'
    # Note: timeout-ms is not changed in testsuite

    dynamic_config.set_values({'TEST_CLIENT_QOS': {'__default__': {'attempts': 2, 'timeout-ms': 9999}}})
    p = await call(service_client)
    assert p['http.request.max_resend_count'] == '2'

    dynamic_config.set_values({'TEST_CLIENT_QOS': {'/test/test': {'attempts': 3, 'timeout-ms': 9998}}})
    p = await call(service_client)
    assert p['http.request.max_resend_count'] == '3'
