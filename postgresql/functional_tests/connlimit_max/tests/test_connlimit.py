import pytest


TESTSUITE_MAX_CONNECTIONS = 100 - 5
STATIC_MAX_CONNECTIONS = 15


@pytest.fixture(autouse=True)
async def clean_uclients(service_client, pgsql):
    await periodic_step(service_client)
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


async def periodic_step(service_client):
    await service_client.run_task('connlimit_watchdog_pg_key_value')


async def test_single_client(monitor_client, service_client, taxi_config):
    assert await get_max_connections(monitor_client) == STATIC_MAX_CONNECTIONS

    taxi_config.set_values({'POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED': True})
    await periodic_step(service_client)

    assert (
        await get_max_connections(monitor_client) == TESTSUITE_MAX_CONNECTIONS
    )


@pytest.mark.config(POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED=True)
async def test_second_client(monitor_client, service_client, pgsql):
    assert (
        await get_max_connections(monitor_client) == TESTSUITE_MAX_CONNECTIONS
    )

    cursor = pgsql['key_value'].cursor()
    cursor.execute(
        'INSERT INTO u_clients (hostname, updated, max_connections) '
        'VALUES (\'xxx\', NOW(), 3)',
    )
    assert (
        await get_max_connections(monitor_client) == TESTSUITE_MAX_CONNECTIONS
    )
    await periodic_step(service_client)
    assert await get_max_connections(monitor_client) == int(
        TESTSUITE_MAX_CONNECTIONS / 2,
    )

    cursor.execute('DELETE FROM u_clients')
    await periodic_step(service_client)
    assert (
        await get_max_connections(monitor_client) == TESTSUITE_MAX_CONNECTIONS
    )
