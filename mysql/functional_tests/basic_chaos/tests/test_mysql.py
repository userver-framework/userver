import asyncio


async def _check_that_restores(client, gate):
    try:
        await gate.wait_for_connections(timeout=30.0)
    except asyncio.TimeoutError:
        assert False, 'Timeout while waiting for restore'

    assert gate.connections_count() >= 1

    for _ in range(30):
        res = await client.delete('/chaos?key=foo')
        if res.status == 200:
            return
        await asyncio.sleep(1)

    assert False, 'Bad results after connection restore'


async def _check_crud(client):
    response = await client.post('/chaos?key=foo&value=bar')
    assert response.status_code == 201

    response = await client.get('/chaos?key=foo')
    assert response.status_code == 200
    assert response.text == 'bar'

    response = await client.delete('/chaos?key=foo')
    assert response.status_code == 200


async def test_mysql_happy(service_client, gate):
    await _check_crud(service_client)


async def test_mysql_disable_reads(service_client, gate):
    gate.to_server_noop()
    result = await service_client.delete('/chaos?key=foo')
    assert result.status == 500
    gate.to_server_pass()
    await _check_that_restores(service_client, gate)


async def test_mysql_disable_writes(service_client, gate):
    gate.to_client_noop()
    result = await service_client.delete('/chaos?key=foo')
    assert result.status == 500
    gate.to_client_pass()
    await _check_that_restores(service_client, gate)


async def test_mysql_close_connections(service_client, gate):
    response = await service_client.get('/chaos?key=foo')
    assert response.status == 404

    await gate.sockets_close()
    await _check_that_restores(service_client, gate)
