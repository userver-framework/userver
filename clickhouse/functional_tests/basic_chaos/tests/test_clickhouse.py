import asyncio
import uuid


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


async def test_clickhouse_happy(service_client, gate):
    await _check_crud(service_client)


async def test_clickhouse_disable_reads(service_client, gate):
    gate.to_server_noop()
    result = await service_client.delete('/chaos?key=foo')
    assert result.status == 500
    gate.to_server_pass()
    await _check_that_restores(service_client, gate)


async def test_clickhouse_disable_writes(service_client, gate):
    gate.to_client_noop()
    result = await service_client.delete('/chaos?key=foo')
    assert result.status == 500
    gate.to_client_pass()
    await _check_that_restores(service_client, gate)


async def test_clickhouse_close_connections(service_client, gate):
    response = await service_client.get('/chaos?key=foo')
    assert response.status == 404

    await gate.sockets_close()
    await _check_that_restores(service_client, gate)


async def test_uuids(service_client, clickhouse):
    user_uuid = '30eec7b3-0b5c-451e-8976-98f62b4c4448'
    big_endian_uuid = '1e455c0b-b3c7-ee30-4844-4c2bf6987689'
    response = await service_client.post(
        '/uuids',
        json={'uuid_mismatched': user_uuid, 'uuid_correct': user_uuid},
    )
    assert response.status == 200

    response = await service_client.get('/uuids')
    assert response.json() == [
        {'uuid_mismatched': user_uuid, 'uuid_correct': user_uuid},
    ]

    db_data = clickhouse['key-value-database'].execute(
        'SELECT uuid_mismatched, uuid_correct FROM uuids',
    )
    assert db_data == [(uuid.UUID(big_endian_uuid), uuid.UUID(user_uuid))]
