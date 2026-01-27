import asyncio

import aiohttp
import pytest_userver.utils.sync as sync


async def test_monitor_port_is_open_before_all_components_are_ready(
    ensure_daemon_started,
    service_daemon_scope,
    service_baseurl,
    monitor_baseurl,
):
    daemon_task = asyncio.create_task(ensure_daemon_started(service_daemon_scope))

    # call /monitor
    async with aiohttp.ClientSession() as session:

        async def is_ready():
            response = await session.get(monitor_baseurl + 'monitor')
            return response.status == 200

        await sync.wait(is_ready, catch=aiohttp.ClientConnectorError)

    # make sure /ping is now ready
    await daemon_task
