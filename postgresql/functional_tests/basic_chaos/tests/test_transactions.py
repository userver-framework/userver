import asyncio

import pytest


# TODO: eliminate copy-paste
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

        # await asyncio.sleep(0.75)
        await asyncio.sleep(3)

    assert False, 'Bad results after connection restore'


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


TESTPOINT_NAMES = (
    'before_trx_begin',
    'after_trx_begin',
    'before_trx_commit',
    'after_trx_commit',
)


# @pytest.mark.skip(reason='Assertion: Timout while waiting for restore')
@pytest.mark.xfail()
@pytest.mark.parametrize('tp_name', TESTPOINT_NAMES)
@pytest.mark.parametrize('attr', ['sockets_close', 'to_right_noop'])
async def test_x(service_client, gate, testpoint, tp_name, attr):
    async def f():
        @testpoint(tp_name)
        async def _hook(data):
            await getattr(gate, attr)()

        response = await service_client.get('/chaos/postgres?type=fill')
        assert response.status == 500
        assert gate.connections_count() == 0

    await service_client.post('/tests/control', data={'testpoints': [tp_name]})
    await f()
    await _check_that_restores(service_client, gate)


TIMEOUT = 6


@pytest.mark.skip(reason='not ready yet')
@pytest.mark.parametrize('tp_name', TESTPOINT_NAMES)
async def test_timeout(service_client, gate, testpoint, tp_name):
    async def f():
        @testpoint(tp_name)
        async def _hook(data):
            await asyncio.sleep(TIMEOUT)

        response = await service_client.get('/chaos/postgres?type=fill')
        assert response.status == 500
        assert gate.connections_count() == 0

    await service_client.post('/tests/control', data={'testpoints': [tp_name]})
    await f()
    await _check_that_restores(service_client, gate)
