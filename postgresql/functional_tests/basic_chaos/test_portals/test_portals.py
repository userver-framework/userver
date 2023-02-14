import logging

import pytest

import utils


PORTAL_URL = '/chaos/postgres?type=portal'
PORTAL_SMALL_TIMEOUT_URL = '/chaos/postgres?type=portal-small-timeout'


logger = logging.getLogger(__name__)


async def test_portal_fine(service_client, gate):
    response = await service_client.get(PORTAL_URL)
    assert response.status == 200
    assert response.json() == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]


TESTPOINT_NAMES = ('after_make_portal', 'after_fetch')


@pytest.mark.parametrize('tp_name', TESTPOINT_NAMES)
async def test_sockets_close(service_client, gate, testpoint, tp_name):
    should_close_sockets = True

    @testpoint(tp_name)
    async def _hook(_data):
        if should_close_sockets:
            await gate.sockets_close()

    logger.debug('Starting "test_sockets_close" check for 500')
    response = await service_client.get(PORTAL_SMALL_TIMEOUT_URL)
    assert response.status == 500
    logger.debug('End of "test_sockets_close" check for 500')

    should_close_sockets = False
    await utils.consume_dead_db_connections(service_client)

    logger.debug('Starting "test_sockets_close" check for 500')
    response = await service_client.get(PORTAL_URL)
    assert response.status_code == 200
    assert response.json() == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    logger.debug('End of "test_sockets_close" check for 200')


DELAY_SECS = 4.0


@pytest.mark.parametrize('tp_name', TESTPOINT_NAMES)
async def test_timeout(service_client, gate, testpoint, tp_name):
    should_delay = True

    @testpoint(tp_name)
    async def _hook(_data):
        if should_delay:
            gate.to_client_delay(DELAY_SECS)

    logger.debug('Starting "test_timeout" check for 500')
    response = await service_client.get(PORTAL_SMALL_TIMEOUT_URL)
    assert response.status == 500
    logger.debug('End of "test_timeout" check for 500')

    should_delay = False
    gate.to_client_pass()
    await utils.consume_dead_db_connections(service_client)

    logger.debug('Starting "test_timeout" check for 200')
    response = await service_client.get(PORTAL_URL)
    assert response.status_code == 200
    assert response.json() == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    logger.debug('End of "test_timeout" check for 200')
