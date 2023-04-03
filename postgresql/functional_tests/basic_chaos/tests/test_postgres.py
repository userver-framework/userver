import logging

import pytest

import utils


DATA_TRANSMISSION_DELAY = 1
BYTES_PER_SECOND_LIMIT = 10
CONNECTION_TIME_LIMIT = 0.4
CONNECTION_LIMIT_JITTER = 0.004
FAILURE_RETRIES = 250
DATA_PARTS_MAX_SIZE = 40
BYTES_TRANSMISSION_LIMIT = 1024

SELECT_URL = '/chaos/postgres?type=select'
SELECT_SMALL_TIMEOUT_URL = '/chaos/postgres?type=select-small-timeout'


logger = logging.getLogger(__name__)


# /// [restore]
async def _check_that_restores(service_client, gate):
    gate.to_server_pass()
    gate.to_client_pass()
    gate.start_accepting()

    await utils.consume_dead_db_connections(service_client)

    logger.debug('Starting "_check_that_restores" wait for 200')
    response = await service_client.get(SELECT_URL)
    assert response.status == 200, 'Bad results after connection restore'
    logger.debug('End of "_check_that_restores" wait for 200')
    # /// [restore]


async def test_pg_fine(service_client, gate):
    response = await service_client.get(SELECT_URL)
    assert response.status == 200


async def test_pg_overload_no_accepts(service_client, gate):
    # Get a connection, if there is no one
    response = await service_client.get(SELECT_URL)
    assert response.status == 200

    await gate.stop_accepting()

    # Use already opened connection
    response = await service_client.get(SELECT_URL)
    assert response.status == 200


# /// [test cc]
async def test_pg_congestion_control(service_client, gate):
    gate.to_server_close_on_data()
    gate.to_client_close_on_data()

    for _ in range(gate.connections_count()):
        response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
        assert response.status == 500

    await _check_that_restores(service_client, gate)
    # /// [test cc]


async def test_close_to_client_limit(service_client, gate):
    for i in range(100, 250, 50):
        gate.to_client_limit_bytes(i)
        response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
        assert response.status == 500, i

    gate.to_client_pass()
    await _check_that_restores(service_client, gate)


async def test_close_to_server_limit(service_client, gate):
    for i in range(100, 250, 50):
        gate.to_server_limit_bytes(i)
        response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
        assert response.status == 500

    gate.to_server_pass()
    await _check_that_restores(service_client, gate)


@pytest.mark.skip(
    reason='Rarely breaks the server, and corrupted data can still be valid',
)
async def test_pg_corupted_response(service_client, gate):
    gate.to_client_corrupt_data()

    for _ in range(gate.connections_count()):
        response = await service_client.get(SELECT_URL)
        assert response.status == 500

    await _check_that_restores(service_client, gate)


async def test_network_delay_sends(service_client, gate):
    gate.to_server_delay(DATA_TRANSMISSION_DELAY)

    logger.debug('Starting "test_network_delay_sends" check for 500')
    response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
    assert response.status == 500
    logger.debug('End of "test_network_delay_sends" check for 500')

    await _check_that_restores(service_client, gate)


async def test_network_delay_recv(service_client, gate):
    gate.to_client_delay(DATA_TRANSMISSION_DELAY)

    response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
    assert response.status == 500

    await _check_that_restores(service_client, gate)


async def test_network_delay(service_client, gate):
    gate.to_server_delay(DATA_TRANSMISSION_DELAY)
    gate.to_client_delay(DATA_TRANSMISSION_DELAY)

    logger.debug('Starting "test_network_delay" check for 500')
    response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
    assert response.status == 500
    logger.debug('End of "test_network_delay" check for 500')

    await _check_that_restores(service_client, gate)


async def test_network_limit_bps_sends(service_client, gate):
    gate.to_server_limit_bps(BYTES_PER_SECOND_LIMIT)

    logger.debug('Starting "test_network_limit_bps_sends" check for 500')
    response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
    assert response.status == 500
    logger.debug('End of "test_network_limit_bps_sends" check for 500')

    await _check_that_restores(service_client, gate)


async def test_network_limit_bps_recv(service_client, gate):
    gate.to_client_limit_bps(BYTES_PER_SECOND_LIMIT)

    logger.debug('Starting "test_network_limit_bps_recv" check for 500')
    response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
    assert response.status == 500
    logger.debug('End of "test_network_limit_bps_recv" check for 500')

    await _check_that_restores(service_client, gate)


async def test_network_limit_bps(service_client, gate):
    gate.to_server_limit_bps(BYTES_PER_SECOND_LIMIT)
    gate.to_client_limit_bps(BYTES_PER_SECOND_LIMIT)

    logger.debug('Starting "test_network_limit_bps" check for 500')
    response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
    assert response.status == 500
    logger.debug('End of "test_network_limit_bps" check for 500')

    await _check_that_restores(service_client, gate)


async def test_network_limit_time_sends(service_client, gate):
    gate.to_server_limit_time(CONNECTION_TIME_LIMIT, CONNECTION_LIMIT_JITTER)

    logger.debug('Starting "test_network_limit_time_sends" check for 500')
    got_error = False
    for _ in range(FAILURE_RETRIES):
        response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
        if response.status != 200:
            got_error = True
            break
    logger.debug('End of "test_network_limit_time_sends" check for 500')

    assert got_error, 'Previous steps unexpectedly finished with success'

    await _check_that_restores(service_client, gate)


async def test_network_limit_time_recv(service_client, gate):
    gate.to_client_limit_time(CONNECTION_TIME_LIMIT, CONNECTION_LIMIT_JITTER)

    logger.debug('Starting "test_network_limit_time_recv" check for 500')
    got_error = False
    for _ in range(FAILURE_RETRIES):
        response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
        if response.status != 200:
            got_error = True
            break
    logger.debug('End of "test_network_limit_time_recv" check for 500')

    assert got_error, 'Previous steps unexpectedly finished with success'

    await _check_that_restores(service_client, gate)


async def test_network_limit_time(service_client, gate):
    gate.to_server_limit_time(CONNECTION_TIME_LIMIT, CONNECTION_LIMIT_JITTER)
    gate.to_client_limit_time(CONNECTION_TIME_LIMIT, CONNECTION_LIMIT_JITTER)

    logger.debug('Starting "test_network_limit_time" check for 500')
    got_error = False
    for _ in range(FAILURE_RETRIES):
        response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
        if response.status != 200:
            got_error = True
            break
    logger.debug('End of "test_network_limit_time" check for 500')

    assert got_error, 'Previous steps unexpectedly finished with success'

    await _check_that_restores(service_client, gate)


async def test_network_smaller_parts_sends(service_client, gate):
    gate.to_server_smaller_parts(DATA_PARTS_MAX_SIZE)

    response = await service_client.get(SELECT_URL)
    assert response.status == 200


async def test_network_smaller_parts_recv(service_client, gate):
    gate.to_client_smaller_parts(DATA_PARTS_MAX_SIZE)

    response = await service_client.get(SELECT_URL)
    assert response.status == 200


async def test_network_smaller_parts(service_client, gate):
    gate.to_server_smaller_parts(DATA_PARTS_MAX_SIZE)
    gate.to_client_smaller_parts(DATA_PARTS_MAX_SIZE)

    response = await service_client.get(SELECT_URL)
    assert response.status == 200


# TODO: timeout does not work!
async def test_network_limit_bytes_sends(service_client, gate):
    gate.to_server_limit_bytes(BYTES_TRANSMISSION_LIMIT)

    logger.debug('Starting "test_network_limit_bytes_sends" check for 500')
    got_error = False
    for _ in range(FAILURE_RETRIES):
        response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
        if response.status != 200:
            got_error = True
            break
    logger.debug('End of "test_network_limit_bytes_sends" check for 500')

    assert got_error, 'Previous steps unexpectedly finished with success'

    await _check_that_restores(service_client, gate)


async def test_network_limit_bytes_recv(service_client, gate):
    gate.to_client_limit_bytes(BYTES_TRANSMISSION_LIMIT)

    logger.debug('Starting "test_network_limit_bytes_recv" check for 500')
    got_error = False
    for _ in range(FAILURE_RETRIES):
        response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
        if response.status != 200:
            got_error = True
            break
    logger.debug('End of "test_network_limit_bytes_recv" check for 500')

    assert got_error, 'Previous steps unexpectedly finished with success'

    await _check_that_restores(service_client, gate)


async def test_network_limit_bytes(service_client, gate):
    gate.to_server_limit_bytes(BYTES_TRANSMISSION_LIMIT)
    gate.to_client_limit_bytes(BYTES_TRANSMISSION_LIMIT)

    logger.debug('Starting "test_network_limit_bytes" check for 500')
    got_error = False
    for _ in range(FAILURE_RETRIES):
        response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
        if response.status != 200:
            got_error = True
            break
    logger.debug('End of "test_network_limit_bytes" check for 500')

    assert got_error, 'Previous steps unexpectedly finished with success'

    await _check_that_restores(service_client, gate)
