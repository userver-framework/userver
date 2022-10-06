import asyncio

import pytest


DATA_TRANSMISSION_DELAY = 0.25
BYTES_PER_SECOND_LIMIT = 1024
CONNECTION_TIME_LIMIT = 0.4
CONNECTION_LIMIT_JITTER = 0.004
FAULRE_RETRIES = 250
DATA_PARTS_SPLIT = 3
BYTES_TRANSMISSION_LIMIT = 40960


# /// [restore]
async def _check_that_restores(service_client, gate):
    try:
        await gate.wait_for_connectons(timeout=5.0)
    except asyncio.TimeoutError:
        assert False, 'Timout while waiting for restore'

    assert gate.connections_count() >= 1

    for _ in range(5):
        response = await service_client.get('/chaos/postgres?type=fill')
        if response.status == 200:
            return

        await asyncio.sleep(0.75)

    assert False, 'Bad results after connection restore'
    # /// [restore]


async def test_pg_fine(service_client, gate):
    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    assert gate.connections_count() > 1

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    assert gate.connections_count() > 1


@pytest.mark.skip(reason='Further tests flap when use the closed connection')
async def test_pg_overload_no_accepts(service_client, gate):
    await gate.stop_accepting()

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    # One broken socket should not affect requests
    await gate.sockets_close(count=1)

    await _check_that_restores(service_client, gate)


@pytest.mark.skip(reason='Assertion: Bad results after connection restore')
async def test_pg_overload_no_reads(service_client, gate):
    gate.to_server_noop()

    for _ in range(gate.connections_count()):
        response = await service_client.get('/chaos/postgres?type=fill')
        assert response.status == 500

    gate.to_server_pass()

    await _check_that_restores(service_client, gate)


@pytest.mark.skip(reason='Assertion: Bad results after connection restore')
async def test_pg_overload_no_writes(service_client, gate):
    gate.to_client_noop()

    for _ in range(gate.connections_count()):
        response = await service_client.get('/chaos/postgres?type=fill')
        assert response.status == 500

    gate.to_client_pass()

    await _check_that_restores(service_client, gate)


@pytest.mark.skip(reason='Assertion: Timout while waiting for restore')
async def test_pg_close_connections(service_client, gate):
    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    await gate.sockets_close()
    await _check_that_restores(service_client, gate)


# /// [test cc]
async def test_pg_congestion_control(service_client, gate):
    gate.to_server_close_on_data()
    gate.to_client_close_on_data()

    for _ in range(gate.connections_count()):
        response = await service_client.get('/chaos/postgres?type=fill')
        assert response.status == 500

    gate.to_server_pass()
    gate.to_client_pass()

    await _check_that_restores(service_client, gate)
    # /// [test cc]


@pytest.mark.skip(reason='Rarely breaks the server')
async def test_pg_corupted_response(service_client, gate):
    gate.to_client_corrupt_data()

    for _ in range(gate.connections_count()):
        response = await service_client.get('/chaos/postgres?type=fill')
        assert response.status == 500

    gate.to_client_pass()
    await _check_that_restores(service_client, gate)


async def test_network_delay_sends(service_client, gate):
    gate.to_server_delay(DATA_TRANSMISSION_DELAY)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200


async def test_network_delay_recv(service_client, gate):
    gate.to_client_delay(DATA_TRANSMISSION_DELAY)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200


async def test_network_delay(service_client, gate):
    gate.to_server_delay(DATA_TRANSMISSION_DELAY)
    gate.to_client_delay(DATA_TRANSMISSION_DELAY)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200


async def test_network_limit_bps_sends(service_client, gate):
    gate.to_server_limit_bps(BYTES_PER_SECOND_LIMIT)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200


async def test_network_limit_bps_recv(service_client, gate):
    gate.to_client_limit_bps(BYTES_PER_SECOND_LIMIT)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200


async def test_network_limit_bps(service_client, gate):
    gate.to_server_limit_bps(BYTES_PER_SECOND_LIMIT)
    gate.to_client_limit_bps(BYTES_PER_SECOND_LIMIT)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200


async def test_network_limit_time_sends(service_client, gate):
    gate.to_server_limit_time(CONNECTION_TIME_LIMIT, CONNECTION_LIMIT_JITTER)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    got_error = False
    for _ in range(FAULRE_RETRIES):
        response = await service_client.get('/chaos/postgres?type=fill')
        if response.status != 200:
            got_error = True
            break

    assert got_error, 'Previous steps unexpectedly finished with success'

    gate.to_server_pass()

    await _check_that_restores(service_client, gate)


@pytest.mark.skip(reason='Service aborted by SIGABRT signal')
async def test_network_limit_time_recv(service_client, gate):
    gate.to_client_limit_time(CONNECTION_TIME_LIMIT, CONNECTION_LIMIT_JITTER)

    got_error = False
    for _ in range(FAULRE_RETRIES):
        response = await service_client.get('/chaos/postgres?type=fill')
        if response.status != 200:
            got_error = True
            break

    assert got_error, 'Previous steps unexpectedly finished with success'

    gate.to_client_pass()

    await _check_that_restores(service_client, gate)


@pytest.mark.skip(reason='Service aborted by SIGABRT signal')
async def test_network_limit_time(service_client, gate):
    gate.to_server_limit_time(CONNECTION_TIME_LIMIT, CONNECTION_LIMIT_JITTER)
    gate.to_client_limit_time(CONNECTION_TIME_LIMIT, CONNECTION_LIMIT_JITTER)

    got_error = False
    for _ in range(FAULRE_RETRIES):
        response = await service_client.get('/chaos/postgres?type=fill')
        if response.status != 200:
            got_error = True
            break

    assert got_error, 'Previous steps unexpectedly finished with success'

    gate.to_server_pass()
    gate.to_client_pass()

    await _check_that_restores(service_client, gate)


async def test_network_smaller_parts_sends(service_client, gate):
    gate.to_server_smaller_parts(DATA_PARTS_SPLIT)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200


async def test_network_smaller_parts_recv(service_client, gate):
    gate.to_client_smaller_parts(DATA_PARTS_SPLIT)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200


async def test_network_smaller_parts(service_client, gate):
    gate.to_server_smaller_parts(DATA_PARTS_SPLIT)
    gate.to_client_smaller_parts(DATA_PARTS_SPLIT)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200


@pytest.mark.skip(reason='Assertion: Timout while waiting for restore')
async def test_network_limit_bytes_sends(service_client, gate):
    gate.to_server_limit_bytes(BYTES_TRANSMISSION_LIMIT)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    got_error = False
    for _ in range(FAULRE_RETRIES):
        response = await service_client.get('/chaos/postgres?type=fill')
        if response.status != 200:
            got_error = True
            break

    assert got_error, 'Previous steps unexpectedly finished with success'

    gate.to_server_pass()

    await _check_that_restores(service_client, gate)


@pytest.mark.skip(reason='Assertion: Timout while waiting for restore')
async def test_network_limit_bytes_recv(service_client, gate):
    gate.to_client_limit_bytes(BYTES_TRANSMISSION_LIMIT)

    got_error = False
    for _ in range(FAULRE_RETRIES):
        response = await service_client.get('/chaos/postgres?type=fill')
        if response.status != 200:
            got_error = True
            break

    assert got_error, 'Previous steps unexpectedly finished with success'

    gate.to_client_pass()

    await _check_that_restores(service_client, gate)


@pytest.mark.skip(reason='Assertion: Timout while waiting for restore')
async def test_network_limit_bytes(service_client, gate):
    gate.to_server_limit_bytes(BYTES_TRANSMISSION_LIMIT)
    gate.to_client_limit_bytes(BYTES_TRANSMISSION_LIMIT)

    got_error = False
    for _ in range(FAULRE_RETRIES):
        response = await service_client.get('/chaos/postgres?type=fill')
        if response.status != 200:
            got_error = True
            break

    assert got_error, 'Previous steps unexpectedly finished with success'

    gate.to_server_pass()
    gate.to_client_pass()

    await _check_that_restores(service_client, gate)
