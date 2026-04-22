import logging

import pytest

logger = logging.getLogger(__name__)


@pytest.fixture
def redis_cluster_ports(_redis_service_settings):
    return _redis_service_settings.sentinel_port


async def _add_cluster(
    service_client,
    redis_cluster_ports,
):
    return await service_client.post(
        '/control',
        json={
            'dbname': 'custom_db',
            'hosts': [
                {
                    'host': 'localhost',
                    'port': redis_cluster_ports,
                }
            ],
            'password': '',
            'sharding_strategy': 'KeyShardTaximeterCrc32',
        },
    )


async def _get_cluster(service_client):
    return await service_client.get(
        '/command',
        params={'dbname': 'custom_db', 'key': 'aaaa'},
    )


async def _delete_cluster(service_client):
    return await service_client.delete(
        '/control',
        params={'dbname': 'custom_db'},
    )


async def test_happy_path(service_client, redis_store, redis_cluster_ports):
    req = await _get_cluster(service_client)
    assert req.status == 403

    req = await _add_cluster(service_client, redis_cluster_ports)
    assert req.status == 200

    redis_store.set('aaaa', 'Hello')

    req = await _get_cluster(service_client)
    assert req.status == 200

    req = await _delete_cluster(service_client)
    assert req.status == 200

    req = await _get_cluster(service_client)
    assert req.status == 403


async def test_duplicated_dbname(service_client, redis_store, redis_cluster_ports):
    req = await _add_cluster(service_client, redis_cluster_ports)
    assert req.status == 200

    req = await _add_cluster(service_client, redis_cluster_ports)
    assert req.status == 409
    req = await _delete_cluster(service_client)
    assert req.status == 200


async def test_empty_hosts(service_client, redis_store, redis_cluster_ports):
    req = await service_client.post(
        '/control',
        json={
            'dbname': 'custom_db',
            'hosts': [],
            'password': '',
            'sharding_strategy': 'RedisCluster',
        },
    )
    assert req.status == 400


# add custom_db and check metrics contains custom_db metrics
async def test_metrics_smoke(service_client, redis_store, redis_cluster_ports, monitor_client, mocked_time):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1

    redis_store.set('aaaa', 'Hello')

    req = await _add_cluster(service_client, redis_cluster_ports)
    assert req.status == 200

    # create some metrics
    req = await _get_cluster(service_client)
    assert req.status == 200

    await service_client.invalidate_caches()

    metrics = await monitor_client.metrics_raw(output_format='pretty')
    assert len(metrics) > 1
    assert metrics.find('custom_db') != -1

    req = await _delete_cluster(service_client)
    assert req.status == 200
