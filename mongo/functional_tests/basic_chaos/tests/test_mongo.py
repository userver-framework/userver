import asyncio

from pytest_userver import chaos


async def _check_that_restores(service_client, gate: chaos.TcpGate):
    gate.to_server_pass()
    gate.to_client_pass()
    gate.start_accepting()

    try:
        await gate.wait_for_connections(timeout=10.0)
    except asyncio.TimeoutError:
        assert False, 'Timout while waiting for restore'

    assert gate.connections_count() >= 1

    for _ in range(10):
        response = await service_client.put('/v1/key-value?key=foo&value=bar')
        if response.status == 200:
            return

        await asyncio.sleep(1)

    assert False, 'Bad results after connection restore'


async def _check_write_ok(service_client):
    response = await service_client.put('/v1/key-value?key=foo&value=bar')
    assert response.status == 200


async def _check_read_ok(service_client):
    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status_code == 200
    assert response.text == 'bar'


async def _check_write_and_read(service_client):
    _check_write_ok(service_client)
    _check_read_ok(service_client)


async def test_mongo_fine(service_client, gate: chaos.TcpGate):
    _check_write_and_read(service_client)


async def test_stop_accepting(service_client, gate: chaos.TcpGate):
    _check_write_and_read(service_client)

    await gate.stop_accepting()

    # should use already opened connections
    _check_write_and_read(service_client)


async def test_close_connection(service_client, gate: chaos.TcpGate):
    gate.to_server_close_on_data()

    response = await service_client.put('/v1/key-value?key=foo&value=bar')
    assert response.status == 500

    await _check_that_restores(service_client, gate)


async def test_block_to_server(service_client, gate: chaos.TcpGate):
    gate.to_server_noop()

    response = await service_client.put('/v1/key-value?key=foo&value=bar')
    assert response.status == 500

    await _check_that_restores(service_client, gate)


async def test_block_to_client(service_client, gate: chaos.TcpGate):
    gate.to_client_noop()

    response = await service_client.put('/v1/key-value?key=foo&value=bar')
    assert response.status == 500

    await _check_that_restores(service_client, gate)
