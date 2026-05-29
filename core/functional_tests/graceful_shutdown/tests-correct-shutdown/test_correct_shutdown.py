import asyncio
from signal import SIGTERM

import aiohttp
import pytest
import pytest_userver.utils.sync as sync


@pytest.mark.uservice_oneshot
async def test_correct_shutdown_with_long_running_request(
    service_daemon_instance,
    service_client,
):
    # This test just checks that the server will not crash on shutdown with active long-running request
    long_running = asyncio.create_task(service_client.get('/test?delay=10s'))

    service_daemon_instance.process.send_signal(SIGTERM)

    async def server_is_down():
        try:
            await service_client.get('/ping')
            return False
        except aiohttp.ClientError:
            return True

    await sync.wait(server_is_down)

    with pytest.raises(aiohttp.ClientError):
        await long_running
