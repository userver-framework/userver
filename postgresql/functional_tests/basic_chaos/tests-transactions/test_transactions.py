import logging

import pytest

import utils


SELECT_URL = '/chaos/postgres?type=select'
SELECT_SMALL_TIMEOUT_URL = '/chaos/postgres?type=select-small-timeout'


logger = logging.getLogger(__name__)


async def test_transaction_fine(service_client, gate, testpoint):
    @testpoint('before_trx_begin')
    async def hook1(_data):
        pass

    @testpoint('after_trx_begin')
    async def hook2(_data):
        pass

    @testpoint('before_trx_commit')
    async def hook3(_data):
        pass

    @testpoint('after_trx_commit')
    async def hook4(_data):
        pass

    response = await service_client.get(SELECT_URL)
    assert response.status == 200

    assert gate.connections_count() >= 1
    assert hook1.times_called == 1
    assert hook2.times_called == 1
    assert hook3.times_called == 1
    assert hook4.times_called == 1


TESTPOINT_NAMES = ('after_trx_begin', 'before_trx_commit')


@pytest.mark.parametrize('tp_name', TESTPOINT_NAMES)
async def test_sockets_close(service_client, gate, testpoint, tp_name):
    should_close_sockets = True

    @testpoint(tp_name)
    async def _hook(_data):
        if should_close_sockets:
            await gate.sockets_close()

    logger.debug('Starting "test_sockets_close" check for 500')
    response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
    assert response.status == 500
    logger.debug('End of "test_sockets_close" check for 500')

    should_close_sockets = False
    await utils.consume_dead_db_connections(service_client)

    logger.debug('Starting "test_sockets_close" check for 200')
    response = await service_client.get(SELECT_URL)
    assert response.status_code == 200
    logger.debug('End of "test_sockets_close" check for 200')


async def test_after_trx_commit(service_client, gate, testpoint):
    @testpoint('after_trx_commit')
    async def _hook(_data):
        await gate.sockets_close()

    logger.debug('Starting "test_after_trx_commit" check for 200')
    # We've already handled the request, thus status code is 200
    response = await service_client.get(SELECT_URL)
    assert response.status_code == 200
    logger.debug('End of "test_after_trx_commit" check for 200')


DELAY_SECS = 4.0


@pytest.mark.parametrize('tp_name', TESTPOINT_NAMES)
async def test_timeout(service_client, gate, testpoint, tp_name):
    should_delay = True

    @testpoint(tp_name)
    async def _hook(_data):
        if should_delay:
            gate.to_client_delay(DELAY_SECS)

    logger.debug('Starting "test_timeout" check for 500')
    response = await service_client.get(SELECT_SMALL_TIMEOUT_URL)
    assert response.status == 500
    logger.debug('End of "test_timeout" check for 500')

    should_delay = False
    gate.to_client_delay(0)
    await utils.consume_dead_db_connections(service_client)

    logger.debug('Starting "test_timeout" check for 200')
    response = await service_client.get(SELECT_URL)
    assert response.status == 200
    logger.debug('End of "test_timeout" check for 200')
