import asyncio
import datetime


async def wait_for_daemon_stop(_global_daemon_store):
    deadline = datetime.datetime.now() + datetime.timedelta(seconds=10)
    while datetime.datetime.now() < deadline and _global_daemon_store.has_running_daemons():
        await asyncio.sleep(0.05)

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
