import pytest


async def test_transaction_fine(service_client, gate, testpoint):
    @testpoint('before_trx_begin')
    async def hook1(data):
        pass

    @testpoint('after_trx_begin')
    async def hook2(data):
        pass

    @testpoint('before_trx_commit')
    async def hook3(data):
        pass

    @testpoint('after_trx_commit')
    async def hook4(data):
        pass

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status == 200

    assert gate.connections_count() > 1
    assert hook1.times_called == 1
    assert hook2.times_called == 1
    assert hook3.times_called == 1
    assert hook4.times_called == 1


async def consume_dead_db_connections(service_client):
    for i in range(5):
        await service_client.get(
            '/chaos/postgres', params={'type': 'fill', 'n': str(i)},
        )


TESTPOINT_NAMES = ('before_trx_begin', 'after_trx_begin', 'before_trx_commit')


@pytest.mark.config(
    POSTGRES_CONNECTION_SETTINGS={
        'key-value-database': {'recent-errors-threshold': 0},
    },
)
@pytest.mark.parametrize('tp_name', TESTPOINT_NAMES)
@pytest.mark.parametrize('attr', ['sockets_close'])
async def test_x(service_client, gate, testpoint, tp_name, attr, taxi_config):
    async def f():
        @testpoint(tp_name)
        async def _hook(data):
            await getattr(gate, attr)()

        response = await service_client.get('/chaos/postgres?type=fill')
        assert response.status == 500
        assert gate.connections_count() == 0

    r = await service_client.post('/tests/control', {'testpoints': [tp_name]})
    assert r.status_code == 200

    await f()

    taxi_config.set_values({'POSTGRES_CONNECTION_SETTINGS': {}})
    r = await service_client.post('/tests/control', {'testpoints': []})
    assert r.status_code == 200

    await consume_dead_db_connections(service_client)

    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status_code == 200


async def test_after_trx_commit(service_client, gate, testpoint, taxi_config):
    @testpoint('after_trx_commit')
    async def _hook(data):
        await gate.sockets_close()

    r = await service_client.post(
        '/tests/control', {'testpoints': ['after_trx_commit']},
    )
    assert r.status_code == 200

    # We've already handled the request, thus status code is 200
    response = await service_client.get('/chaos/postgres?type=fill')
    assert response.status_code == 200


DELAY_SECS = 4.0


@pytest.mark.skip(reason='not ready yet')
@pytest.mark.parametrize('tp_name', TESTPOINT_NAMES)
async def test_timeout(service_client, gate, testpoint, tp_name):
    async def f():
        @testpoint(tp_name)
        async def _hook(data):
            gate.to_client_delay(DELAY_SECS)

        response = await service_client.get('/chaos/postgres?type=fill')
        assert response.status == 500

    r = await service_client.post('/tests/control', {'testpoints': [tp_name]})
    assert r.status_code == 200

    await f()

    r = await service_client.post('/tests/control', {'testpoints': []})
    assert r.status_code == 200
