import asyncio
import logging

from pytest_userver import chaos


logger = logging.getLogger(__name__)


async def _check_that_restores(service_client, gate: chaos.TcpGate):
    logger.info('mongotest starts "_check_that_restores"')
    gate.to_server_pass()
    gate.to_client_pass()
    gate.start_accepting()

    try:
        await gate.wait_for_connections(timeout=10.0)
    except asyncio.TimeoutError:
        assert False, 'Timeout while waiting for restore'

    assert gate.connections_count() >= 1

    for _ in range(10):
        response = await service_client.put('/v1/key-value?key=foo&value=bar')
        if response.status == 200:
            logger.info('mongotest ends "_check_that_restores"')
            return

        await asyncio.sleep(1)

    assert False, 'Bad results after connection restore'


async def _check_write_ok(service_client):
    logger.info('mongotest starts "_check_write_ok"')
    response = await service_client.put('/v1/key-value?key=foo&value=bar')
    assert response.status == 200
    logger.info('mongotest ends "_check_write_ok"')


async def _check_read_ok(service_client):
    logger.info('mongotest starts "_check_read_ok"')
    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status_code == 200
    assert response.text == 'bar'
    logger.info('mongotest ends "_check_read_ok"')


async def _check_write_and_read(service_client):
    await _check_write_ok(service_client)
    await _check_read_ok(service_client)


async def test_mongo_fine(service_client, gate: chaos.TcpGate):
    logger.info('mongotest starts "test_mongo_fine"')
    await _check_write_and_read(service_client)
    logger.info('mongotest finishes "test_mongo_fine"')


async def test_stop_accepting(service_client, gate: chaos.TcpGate):
    await _check_write_and_read(service_client)

    await gate.stop_accepting()

    # should use already opened connections
    await _check_write_and_read(service_client)


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
