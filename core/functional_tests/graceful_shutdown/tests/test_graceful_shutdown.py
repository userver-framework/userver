import asyncio

import pytest_userver.utils.sync as sync


async def wait_for_daemon_stop(_global_daemon_store):
    def is_ready():
        return not _global_daemon_store.has_running_daemons()

    await sync.wait(is_ready)

    assert not _global_daemon_store.has_running_daemons(), 'Daemon has not stopped'
    await _global_daemon_store.aclose()


async def test_graceful_shutdown_timer(
    service_client,
    monitor_client,
    _global_daemon_store,
):
    response = await service_client.get('/ping')
    assert response.status == 200

    response = await monitor_client.post('/sigterm')
    assert response.status == 200

    response = await service_client.get('/ping')
    assert response.status == 500

    await asyncio.sleep(2)
    # Check that the service is still alive.
    response = await service_client.get('/ping')
    assert response.status == 500

    # After a couple more seconds, the service will start shutting down.
    await wait_for_daemon_stop(_global_daemon_store)
