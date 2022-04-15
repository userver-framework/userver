# /// [Testpoint - fixture]
async def test_basic(test_service_client, testpoint):
    @testpoint('simple-testpoint')
    def simple_testpoint(data):
        assert data == {'payload': 'Hello, world!'}

    response = await test_service_client.get('/testpoint')
    assert response.status == 200
    assert simple_testpoint.times_called == 1
    # /// [Testpoint - fixture]


# /// [Sample TESTPOINT_CALLBACK usage python]
async def test_injection(test_service_client, testpoint):
    @testpoint('injection-point')
    def injection_point(data):
        return {'value': 'injected'}

    response = await test_service_client.get('/testpoint')
    assert response.status == 200
    assert response.json() == {'value': 'injected'}

    # testpoint supports callqueue interface
    assert injection_point.times_called == 1
    # /// [Sample TESTPOINT_CALLBACK usage python]
