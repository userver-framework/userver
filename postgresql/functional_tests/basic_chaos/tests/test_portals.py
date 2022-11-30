import pytest


async def test_portal_fine(service_client, gate, testpoint):
    response = await service_client.get('/chaos/postgres?type=portal')
    assert response.status == 200
    assert response.json() == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]


async def consume_dead_db_connections(service_client):
    for i in range(10):
        await service_client.get(
            '/chaos/postgres', params={'type': 'fill', 'n': str(i)},
        )


TESTPOINT_NAMES = ('after_make_portal', 'after_fetch')


@pytest.mark.config(
    POSTGRES_CONNECTION_SETTINGS={
        'key-value-database': {'recent-errors-threshold': 0},
    },
)
@pytest.mark.parametrize('tp_name', TESTPOINT_NAMES)
@pytest.mark.parametrize('attr', ['sockets_close'])
async def test_x(
        service_client, gate, testpoint, tp_name, attr, dynamic_config,
):
    async def f():
        @testpoint(tp_name)
        async def _hook(data):
            await getattr(gate, attr)()

        response = await service_client.get('/chaos/postgres?type=portal')
        assert response.status == 500
        assert gate.connections_count() == 0

    r = await service_client.post('/tests/control', {'testpoints': [tp_name]})
    assert r.status_code == 200

    await f()

    dynamic_config.set_values({'POSTGRES_CONNECTION_SETTINGS': {}})
    r = await service_client.post('/tests/control', {'testpoints': []})
    assert r.status_code == 200

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
    async def f():
        @testpoint(tp_name)
        async def _hook(data):
            gate.to_client_delay(DELAY_SECS)

        response = await service_client.get('/chaos/postgres?type=portal')
        assert response.status == 500

    r = await service_client.post('/tests/control', {'testpoints': [tp_name]})
    assert r.status_code == 200

    await f()

    r = await service_client.post('/tests/control', {'testpoints': []})
    assert r.status_code == 200

    gate.to_client_pass()

    await consume_dead_db_connections(service_client)

    response = await service_client.get('/chaos/postgres?type=portal')
    assert response.status_code == 200
    assert response.json() == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
