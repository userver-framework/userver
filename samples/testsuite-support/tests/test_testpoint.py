import pytest
import pytest_userver.plugins.testpoint

import testsuite.plugins.testpoint


# /// [Testpoint - fixture]
async def test_basic(service_client, testpoint: testsuite.plugins.testpoint.TestpointFixture):
    @testpoint('simple-testpoint')
    def simple_testpoint(data: dict):
        assert data == {'payload': 'Hello, world!'}

    response = await service_client.get('/testpoint')
    assert response.status == 200
    assert 'application/json' in response.headers['Content-Type']
    assert simple_testpoint.times_called == 1
    # /// [Testpoint - fixture]


# /// [Sample TESTPOINT_CALLBACK usage python]
async def test_injection(service_client, testpoint: testsuite.plugins.testpoint.TestpointFixture):
    @testpoint('injection-point')
    def injection_point(data: dict):
        return {'value': 'injected'}

    response = await service_client.get('/testpoint')
    assert response.status == 200
    assert 'application/json' in response.headers['Content-Type']
    assert response.json() == {'value': 'injected'}

    # testpoint supports callqueue interface
    assert injection_point.times_called == 1
    # /// [Sample TESTPOINT_CALLBACK usage python]


async def test_disabled_testpoint(service_client, testpoint):
    response = await service_client.get('/testpoint')
    assert response.status == 200

    @testpoint('injection-point')
    def injection_point(data: dict):
        return {'value': 'injected'}

    # /// [Unregistered testpoint usage]
    with pytest.raises(
        pytest_userver.plugins.testpoint.UnregisteredTestpointError,
    ):
        assert injection_point.times_called == 0
    # /// [Unregistered testpoint usage]

    await service_client.update_server_state()
    assert injection_point.times_called == 0


async def test_manual_testpoint_registration(service_client, testpoint):
    # /// [Manual registration]
    @testpoint('injection-point')
    def injection_point(data):
        return {'value': 'injected'}

    await service_client.update_server_state()
    assert injection_point.times_called == 0
    # /// [Manual registration]

    response = await service_client.get('/testpoint')
    assert response.status == 200

    assert injection_point.times_called == 1
