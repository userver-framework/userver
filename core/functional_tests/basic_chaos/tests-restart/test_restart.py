import pytest_userver.utils.sync as sync


async def wait_for_daemon_stop(_global_daemon_store):
    def is_ready():
        return not _global_daemon_store.has_running_daemons()

    await sync.wait(is_ready)

    assert not _global_daemon_store.has_running_daemons(), 'Daemon has not stopped'
    await _global_daemon_store.aclose()


async def test_restart(monitor_client, service_client, _global_daemon_store):
    response = await monitor_client.get('/restart', params={'delay': '1'})
    assert response.status == 200

    response = await service_client.get('/ping')
    assert response.status == 500

    await wait_for_daemon_stop(_global_daemon_store)
