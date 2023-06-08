import logging
import socket

import pytest

from pytest_userver import chaos

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


@pytest.mark.skip(reason='response.status == 200')
async def test_network_delay_sends(service_client, gate):
    gate.to_server_delay(DATA_TRANSMISSION_DELAY)

    logger.debug('Starting "test_network_delay_sends" check for 500')
    response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
    assert response.status == 500
    logger.debug('End of "test_network_delay_sends" check for 500')

    await _check_that_restores(service_client, gate)


@pytest.mark.skip(reason='response.status == 200')
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


async def _intercept_server_terminated(
        loop, socket_from: socket.socket, socket_to: socket.socket,
) -> None:
    error_msg = (
        b'E\x00\x00\x00tSFATAL\x00VFATAL\x00C57P01\x00'
        b'Mterminating connection due to administrator command\x00'
        b'Fpostgres.c\x00L3218\x00RProcessInterrupts\x00\x00'
    )
    ready_for_query = b'Z\x00\x00\x00\x05'

    # Wait until we get the entire server response,
    # then send an error message instead of 'Z' and
    # close the socket immediately after that.
    data = b''
    n = -1
    while n < 0:
        data += await loop.sock_recv(socket_from, 4096)
        n = data.find(ready_for_query)
    await loop.sock_sendall(socket_to, data[:n])
    await loop.sock_sendall(socket_to, error_msg)
    raise chaos.GateInterceptException('Closing socket after error')


async def test_close_with_error(service_client, gate, testpoint):
    should_close = False

    @testpoint('after_trx_begin')
    async def _hook(_data):
        if should_close:
            gate.set_to_client_interceptor(_intercept_server_terminated)

    response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
    assert response.status == 200

    should_close = True
    gate.set_to_client_interceptor(_intercept_server_terminated)

    response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
    assert response.status == 500

    should_close = False
    await _check_that_restores(service_client, gate)
