import asyncio

import pytest


async def _check_that_restores(client, gate):
    try:
        await gate.wait_for_connections(timeout=10.0)
    except asyncio.TimeoutError:
        assert False, 'Timeout while waiting for restore'

    assert gate.connections_count() >= 1

    for _ in range(10):
        res = await client.delete('/chaos?key=foo')
        if res.status == 200:
            return
        await asyncio.sleep(1)

    assert False, 'Bad results after connection restore'


async def _check_crud(client):
    response = await client.post('/chaos?key=foo&value=bar')
    assert response.status == 201

    response = await client.get('/chaos?key=foo')
    assert response.status == 200
    assert response.text == 'bar'

    response = await client.post('/chaos?key=foo&value=bar2')
    assert response.status == 409

    response = await client.delete('/chaos?key=foo')
    assert response.status == 200


async def test_redis_happy(service_client, sentinel_gate, gate):
    await _check_crud(service_client)


@pytest.mark.skip(reason='Flaky test TAXICOMMON-6075')
async def test_smaller_parts(service_client, sentinel_gate, gate):
    gate.to_server_smaller_parts(20)
    gate.to_client_smaller_parts(20)
    await _check_crud(service_client)


@pytest.mark.skip(reason='Flaky test TAXICOMMON-6075')
async def test_redis_disable_reads(service_client, sentinel_gate, gate):
    gate.to_server_noop()
    result = await service_client.delete('/chaos?key=foo')
    assert result.status == 500
    gate.to_server_pass()
    await _check_that_restores(service_client, gate)


async def test_redis_disable_writes(service_client, sentinel_gate, gate):
    gate.to_client_noop()
    result = await service_client.delete('/chaos?key=foo')
    assert result.status == 500
    gate.to_client_pass()
    await _check_that_restores(service_client, gate)


async def test_redis_close_connections(service_client, sentinel_gate, gate):
    response = await service_client.get('/chaos?key=foo')
    assert response.status == 404

    await gate.sockets_close()
    await _check_that_restores(service_client, gate)
