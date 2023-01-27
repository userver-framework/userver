import pytest


async def test_portal_fine(service_client, gate):
    response = await service_client.get('/chaos/postgres?type=portal')
    assert response.status == 200
    assert response.json() == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]


async def consume_dead_db_connections(service_client):
    for i in range(10):
        await service_client.get(
            '/chaos/postgres', params={'type': 'fill', 'n': str(i)},
        )


TESTPOINT_NAMES = ('after_make_portal', 'after_fetch')


@pytest.mark.parametrize('tp_name', TESTPOINT_NAMES)
async def test_sockets_close(service_client, gate, testpoint, tp_name):
    should_close_sockets = True

    @testpoint(tp_name)
    async def _hook(_data):
        if should_close_sockets:
            await gate.sockets_close()

    response = await service_client.get('/chaos/postgres?type=portal')
    assert response.status == 500
    assert gate.connections_count() == 0

    should_close_sockets = False
    await consume_dead_db_connections(service_client)

    response = await service_client.get('/chaos/postgres?type=portal')
    assert response.status_code == 200
    assert response.json() == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]


DELAY_SECS = 4.0


@pytest.mark.config(
    POSTGRES_CONNECTION_SETTINGS={
        'key-value-database': {'recent-errors-threshold': 10000},
    },
)
@pytest.mark.parametrize('tp_name', TESTPOINT_NAMES)
async def test_timeout(service_client, gate, testpoint, tp_name):
    should_delay = True

    @testpoint(tp_name)
    async def _hook(_data):
        if should_delay:
            gate.to_client_delay(DELAY_SECS)

    response = await service_client.get('/chaos/postgres?type=portal')
    assert response.status == 500

    should_delay = False
    gate.to_client_pass()
    await consume_dead_db_connections(service_client)

    response = await service_client.get('/chaos/postgres?type=portal')
    assert response.status_code == 200
    assert response.json() == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
