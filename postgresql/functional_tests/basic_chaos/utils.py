# /// [consume_dead_db_connections]
import asyncio
import logging

SELECT_URL = '/chaos/postgres?type=select'
MAX_POOL_SIZE = 1  # should be in sync with ./static_config.yaml


logger = logging.getLogger(__name__)


async def consume_dead_db_connections(service_client):
    logger.debug('Starting "consume_dead_db_connections"')
    await asyncio.gather(
        *[service_client.get(SELECT_URL) for _ in range(MAX_POOL_SIZE * 2)],
    )
    logger.debug('End of "consume_dead_db_connections"')

    logger.debug('Starting "consume_dead_db_connections" check for 200')
    results_list = await asyncio.gather(
        *[service_client.get(SELECT_URL) for _ in range(MAX_POOL_SIZE)],
    )
    for result in results_list:
        assert result.status_code == 200
    logger.debug('End of "consume_dead_db_connections" check for 200')
    # /// [consume_dead_db_connections]
