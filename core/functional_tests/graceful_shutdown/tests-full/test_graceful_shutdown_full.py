import asyncio
from signal import SIGTERM

import aiohttp
import pytest
import pytest_userver.utils.sync as sync


@pytest.mark.uservice_oneshot
async def test_graceful_shutdown_full(
    service_daemon_instance,
    service_baseurl,
    service_client,
    dynamic_config,
):
    headers = {'x-envoy-immediate-health-check-fail': ['true']}
    dynamic_config.set_values({'GRACEFUL_SHUTDOWN_HEADERS': {'enabled': True, 'headers': headers}})
    await service_client.update_server_state()

    response = await service_client.get('/ping')
    assert response.status == 200

    response = await service_client.get('/test')
    assert response.status == 200
    for header, _ in headers.items():
        assert response.headers.get(header) is None

    service_daemon_instance.process.send_signal(SIGTERM)

    async def is_failing():
        response = await service_client.get('/ping')
        return response.status == 500

    await sync.wait(is_failing, relax_period_seconds=0.1)

    # Stage 1: ping failing, new requests accepted
    response = await service_client.get('/test')
    assert response.status == 200
    for header, values in headers.items():
        assert response.headers.get(header) == ', '.join(values)

    await asyncio.sleep(1)

    # Check that the service is still alive.
    response = await service_client.get('/ping')
    assert response.status == 500

    # Stage 2: new requests rejected, old requests are still running
    # Start a delayed request
    delayed_request = asyncio.create_task(service_client.get('/test?delay=5s'))

    await asyncio.sleep(5)

    # We can't rely on service_client here because it pools open HTTP connections to the server
    # and might issue new requests via those pooled connections.
    # So, to check that listeners are closed we need to explicitly create a new HTTP connection
    async with aiohttp.ClientSession(service_baseurl) as session:
        with pytest.raises(aiohttp.ClientError):
            await session.get('/test')
        with pytest.raises(aiohttp.ClientError):
            await session.get('/ping')

    response = await delayed_request
    assert response.status == 200
    for header, values in headers.items():
        assert response.headers.get(header) == ', '.join(values)

    # After a couple more seconds, the service will start shutting down.
    service_daemon_instance.process.wait()
