import pytest
import pytest_userver.utils.sync as sync

TESTSUITE_MAX_CONNECTIONS = 100 - 5
STATIC_MAX_CONNECTIONS = 15
BOOTSTRAP_POOL_SIZE = 1
FOREIGN_CLIENTS = 9


@pytest.fixture(autouse=True)
async def clean_uclients(service_client, pgsql):
    yield
    cursor = pgsql['key_value'].cursor()
    cursor.execute('DELETE FROM u_clients')
    await periodic_step(service_client)


async def get_max_connections(monitor_client) -> int:
    metric = await monitor_client.single_metric(
        'postgresql.connections.max',
        labels={'postgresql_database': 'pg_key_value'},
    )
    return metric.value


async def get_active_connections(monitor_client) -> int:
    metric = await monitor_client.single_metric(
        'postgresql.connections.active',
        labels={'postgresql_database': 'pg_key_value'},
    )
    return metric.value


async def wait_for_active_connections(monitor_client, expected: int):
    async def has_expected_active_connections():
        return await get_active_connections(monitor_client) == expected

    await sync.wait(
        has_expected_active_connections,
        relax_period_seconds=0.05,
        total_wait_seconds=1.0,
        failure_msg=f'PostgreSQL pool did not reach {expected} active connections',
    )


async def periodic_step(service_client):
    await service_client.run_task('connlimit_watchdog_pg_key_value_0')


@pytest.mark.uservice_oneshot
async def test_bootstrap_then_warm_up(
    monitor_client,
    service_client,
    pgsql,
):
    assert await get_max_connections(monitor_client) == BOOTSTRAP_POOL_SIZE
    assert await get_active_connections(monitor_client) == BOOTSTRAP_POOL_SIZE

    cursor = pgsql['key_value'].cursor()
    cursor.executemany(
        """
        INSERT INTO u_clients (hostname, updated, max_connections)
        VALUES (%s, NOW(), 0)
        """,
        [(f'foreign-client-{index}',) for index in range(FOREIGN_CLIENTS)],
    )
    await periodic_step(service_client)

    expected_connlimit = TESTSUITE_MAX_CONNECTIONS // (FOREIGN_CLIENTS + 1)
    await wait_for_active_connections(monitor_client, expected_connlimit)
    assert await get_max_connections(monitor_client) == expected_connlimit


async def test_single_client(monitor_client, service_client, taxi_config):
    taxi_config.set_values({'POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED': False})
    await periodic_step(service_client)

    assert await get_max_connections(monitor_client) == STATIC_MAX_CONNECTIONS

    taxi_config.set_values({'POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED': True})
    await periodic_step(service_client)

    assert await get_max_connections(monitor_client) == TESTSUITE_MAX_CONNECTIONS


@pytest.mark.config(POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED=True)
async def test_second_client(monitor_client, service_client, pgsql):
    await periodic_step(service_client)
    assert await get_max_connections(monitor_client) == TESTSUITE_MAX_CONNECTIONS

    cursor = pgsql['key_value'].cursor()
    cursor.execute(
        "INSERT INTO u_clients (hostname, updated, max_connections) VALUES ('xxx', NOW(), 3)",
    )
    assert await get_max_connections(monitor_client) == TESTSUITE_MAX_CONNECTIONS
    await periodic_step(service_client)
    assert await get_max_connections(monitor_client) == int(
        TESTSUITE_MAX_CONNECTIONS / 2,
    )

    cursor.execute('DELETE FROM u_clients')
    await periodic_step(service_client)
    assert await get_max_connections(monitor_client) == TESTSUITE_MAX_CONNECTIONS
