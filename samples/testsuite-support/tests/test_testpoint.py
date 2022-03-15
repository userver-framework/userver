async def test_basic(test_service_client, testpoint):
    @testpoint('simple-testpoint')
    def simple_testopint(data):
        assert data == {'payload': 'Hello, world!'}

    response = await test_service_client.get('/testpoint')
    assert response.status == 200
    assert simple_testopint.times_called == 1


async def test_injection(test_service_client, testpoint):
    @testpoint('injection-point')
    def injection_point(data):
        return {'value': 'injected'}

    response = await test_service_client.get('/testpoint')
    assert response.status == 200
    assert response.json() == {'value': 'injected'}

    assert injection_point.times_called == 1
