import pytest_userver.client
import pytest_userver.dynconf


async def _select(service_client: pytest_userver.client.Client) -> None:
    response = await service_client.post(
        'ydb/select-rows',
        json={
            'service': 'srv',
            'channels': [1, 2, 3],
            'created': '2019-10-30T11:20:00+00:00',
        },
    )
    assert response.status_code == 200


def _account_ok(differ, dbname: str) -> int:
    # The retry budget is reported under the logical 'ydb_database' label but is
    # charged on the connection's driver. The select-rows handler caches the
    # table client for 'sampledb' at construction; routing it to 'sampledb2'
    # charges sampledb2's driver, so account_ok shows up under 'sampledb2' (it
    # stays at 0 there without routing) -- the cached-client redirect that
    # YDB_DATABASE_ROUTING must perform.
    return differ.value_at('account_ok', add_labels={'ydb_database': dbname})


async def test_database_routing_redirects_cached_client(
    dynamic_config: pytest_userver.dynconf.DynamicConfig,
    service_client: pytest_userver.client.Client,
    monitor_client: pytest_userver.client.ClientMonitor,
) -> None:
    requests_count = 5

    # 1. No routing: requests are served by the home database 'sampledb'.
    dynamic_config.set(YDB_DATABASE_ROUTING={})
    await service_client.update_server_state()
    async with monitor_client.metrics_diff(prefix='ydb.retry_budget') as differ:
        for _ in range(requests_count):
            await _select(service_client)
    assert _account_ok(differ, 'sampledb') > 0
    assert _account_ok(differ, 'sampledb2') == 0

    # 2. Route sampledb -> sampledb2: the already-cached table client must now
    #    use sampledb2's connection, so sampledb2's driver retry budget gets
    #    charged (it was idle, at 0, without routing).
    dynamic_config.set(YDB_DATABASE_ROUTING={'sampledb': 'sampledb2'})
    await service_client.update_server_state()
    async with monitor_client.metrics_diff(prefix='ydb.retry_budget') as differ:
        for _ in range(requests_count):
            await _select(service_client)
    assert _account_ok(differ, 'sampledb2') > 0

    # 3. Revert: routing removed -> back to the home database.
    dynamic_config.set(YDB_DATABASE_ROUTING={})
    await service_client.update_server_state()
    async with monitor_client.metrics_diff(prefix='ydb.retry_budget') as differ:
        for _ in range(requests_count):
            await _select(service_client)
    assert _account_ok(differ, 'sampledb') > 0
    assert _account_ok(differ, 'sampledb2') == 0


async def test_database_routing_invalid_target_is_ignored(
    dynamic_config: pytest_userver.dynconf.DynamicConfig,
    service_client: pytest_userver.client.Client,
    monitor_client: pytest_userver.client.ClientMonitor,
) -> None:
    # Routing to a database that is not configured must be ignored (log + keep
    # the home connection); the service keeps working on 'sampledb'.
    dynamic_config.set(YDB_DATABASE_ROUTING={'sampledb': 'no-such-db'})
    await service_client.update_server_state()
    async with monitor_client.metrics_diff(prefix='ydb.retry_budget') as differ:
        for _ in range(5):
            await _select(service_client)
    assert _account_ok(differ, 'sampledb') > 0
    assert _account_ok(differ, 'sampledb2') == 0
