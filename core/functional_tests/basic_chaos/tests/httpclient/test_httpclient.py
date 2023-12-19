import pytest


@pytest.fixture(name='call')
def _call(service_client, for_client_gate_port, gate):
    async def _call(htype='common', headers=None, **args):
        return await service_client.get(
            '/chaos/httpclient',
            params={'type': htype, 'port': str(for_client_gate_port), **args},
            headers=headers or {},
        )

    return _call


@pytest.fixture(name='ok_mock')
def _ok_mock(mockserver):
    @mockserver.handler('/test')
    async def mock(_request):
        return mockserver.make_response('OK!')

    return mock


async def test_ok(call, ok_mock):
    response = await call()
    assert response.status == 200
    assert response.text == 'OK!'


async def test_stop_accepting(call, gate, ok_mock):
    response = await call()
    assert response.status == 200
    assert gate.connections_count() >= 1

    await gate.stop_accepting()
    await gate.sockets_close()  # close keepalive connections
    assert gate.connections_count() == 0

    response = await call()
    assert response.status == 500
    assert gate.connections_count() == 0

    gate.start_accepting()

    response = await call()
    assert response.status == 200
    assert gate.connections_count() >= 1


async def test_close_after_headers(call, gate, mockserver):
    @mockserver.handler('/test')
    async def _mock(request):
        if should_close_sockets:
            await gate.sockets_close()
        return mockserver.make_response('OK!')

    should_close_sockets = True
    response = await call()
    assert response.status == 500

    should_close_sockets = False
    response = await call()
    assert response.status == 200


async def test_required_headers(call, gate, ok_mock):
    required_headers = ['X-YaRequestId', 'X-YaSpanId', 'X-YaTraceId']

    response = await call()
    assert response.status == 200
    assert all(key in response.headers for key in required_headers)

    await gate.stop_accepting()
    await gate.sockets_close()  # close keepalive connections
    assert gate.connections_count() == 0

    response = await call()
    assert response.status == 500
    assert all(key in response.headers for key in required_headers)
    assert gate.connections_count() == 0
