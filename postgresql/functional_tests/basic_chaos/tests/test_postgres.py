import pytest


async def test_basic(service_client, gate):
    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    assert gate.connections_count() > 1

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    assert gate.connections_count() > 1


@pytest.mark.skip(reason='it takes 30 seconds for the driver to restore')
async def test_connections_broke(service_client, gate):
    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    await gate.sockets_close()
    await gate.wait_for_connectons()  # TODO: driver is slow

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    assert gate.connectons_count() > 1


@pytest.mark.skip(reason='hangs infinitely')
async def test_responses_lost(service_client, gate):
    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    gate.to_right_noop()

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 500

    assert gate.connectons_count() > 1
