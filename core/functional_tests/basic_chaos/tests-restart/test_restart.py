import asyncio
import datetime


async def wait_for_daemon_stop(_global_daemon_store):
    deadline = datetime.datetime.now() + datetime.timedelta(seconds=10)
    while datetime.datetime.now() < deadline and _global_daemon_store.has_running_daemons():
        await asyncio.sleep(0.05)

    assert not _global_daemon_store.has_running_daemons(), 'Daemon has not stopped'
    await _global_daemon_store.aclose()


async def test_restart(monitor_client, service_client, _global_daemon_store):
    response = await monitor_client.get('/restart', params={'delay': '1'})
    assert response.status == 200

    response = await service_client.get('/ping')
    assert response.status == 500

    await wait_for_daemon_stop(_global_daemon_store)
