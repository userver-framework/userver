import pytest


MULTIPLE_CHUNKS_BODY_SIZE = 1000000


@pytest.fixture
def call(service_client, for_client_gate_port):
    async def _call(**args):
        return await service_client.get(
            '/chaos/httpclient/stream',
            params={'port': str(for_client_gate_port), **args},
        )

    return _call


async def test_ok(call, gate, mockserver):
    body = 'x' * MULTIPLE_CHUNKS_BODY_SIZE

    @mockserver.handler('/test')
    async def mock(request):
        return mockserver.make_response(body)

    response = await call()
    assert response.status == 200
    assert response.text == body
    assert mock.times_called == 1


async def test_0_timeout(call, gate, mockserver):
    response = await call(timeout=0)
    assert response.status == 500


async def test_stop_accepting(call, gate, mockserver):
    @mockserver.handler('/test')
    async def mock(request):
        return mockserver.make_response('OK!')

    response = await call()
    assert response.status == 200
    assert gate.connections_count() >= 1
    assert mock.times_called == 1

    await gate.stop_accepting()
    await gate.sockets_close()  # close keepalive connections
    assert gate.connections_count() == 0

    response = await call()
    assert response.status == 500
    assert gate.connections_count() == 0

    gate.start_accepting()

    for _ in range(3):
        response = await call()
        if response.status == 200:
            assert gate.connections_count() >= 1
            assert mock.times_called == 3
            return

    assert False


async def test_close_after_headers(call, gate, mockserver):
    @mockserver.handler('/test')
    async def mock(request):
        if on:
            await gate.sockets_close()
        return mockserver.make_response('OK!')

    on = True
    response = await call()
    assert response.status == 500

    on = False
    # It takes some time in Arcadia CI to restore connection
    response = await call(timeout=30)
    assert response.status == 200
    assert mock.times_called > 1


async def test_timeout(call, gate, mockserver):
    @mockserver.handler('/test')
    async def mock(request):
        return mockserver.make_response('OK!')

    response = await call()
    assert response.status == 200

    await gate.stop_accepting()
    await gate.sockets_close()  # close keepalive connections

    response = await call(timeout=1)
    assert response.status == 500

    gate.start_accepting()

    response = await call()
    assert response.status == 200
    assert mock.times_called > 1


async def test_network_failure(call, gate, mockserver):
    stop = True

    @mockserver.handler('/test')
    async def mock(request):
        if stop:
            await gate.stop_accepting()
            await gate.sockets_close()

        return mockserver.make_response('OK!')

    response = await call(timeout=1)
    assert response.status == 500

    gate.start_accepting()
    stop = False

    response = await call()
    assert response.status == 200
    assert mock.times_called > 1
