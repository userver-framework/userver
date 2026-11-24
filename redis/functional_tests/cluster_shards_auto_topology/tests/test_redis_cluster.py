import asyncio

import pytest_userver.utils.sync as sync

from taxi.uservices.userver.redis.functional_tests.pytest_redis_cluster_topology_plugin import pytest_plugin as topology

REDIS_PORT = 6379
FAILOVER_DEADLINE_SEC = 30  # maximum time allowed to finish failover


async def test_happy_path(service_client, redis_cluster_topology):
    post_reqs = [
        service_client.post(
            '/redis-cluster',
            params={'key': f'key{i}', 'value': 'abc'},
        )
        for i in range(topology.KEYS_SEQ_LEN)
    ]
    assert all(res.status == 201 for res in await asyncio.gather(*post_reqs))

    await topology.assert_read_all_keys(service_client, 'key', 'abc')


async def _check_write_all_slots(service_client, key_prefix, value):
    post_reqs = [
        service_client.post(
            '/redis-cluster',
            params={'key': f'{key_prefix}{i}', 'value': value},
        )
        for i in range(topology.KEYS_SEQ_LEN)
    ]
    return all(res.status != 500 for res in await asyncio.gather(*post_reqs))


async def test_hard_failover(service_client, redis_cluster_topology):
    # Write keys across all shards before failover.
    assert await _check_write_all_slots(service_client, 'hf_key1', 'abc')

    # Start the failover
    await redis_cluster_topology.get_masters()[0].stop()

    # wait until service detect that shard 0 is broken
    # Failover starts in ~10 seconds
    async def is_ready():
        return await _check_write_all_slots(
            service_client,
            'hf_key2',
            'cde',
        )

    await sync.wait(is_ready)

    # Check that existing data and new writes are readable after failover.
    await topology.assert_read_all_keys(service_client, 'hf_key1', 'abc')
    await topology.assert_read_all_keys(service_client, 'hf_key2', 'cde')


async def test_add_shard(service_client, redis_cluster_topology):
    # Keep data separate from test_hard_failover in the shared cluster.
    assert await _check_write_all_slots(service_client, 'add_shard_before_', 'abc')

    await redis_cluster_topology.add_shard()

    async def is_ready():
        return await _check_write_all_slots(
            service_client,
            'add_shard_after_',
            'cde',
        )

    await sync.wait(is_ready)

    # Read migrated data and new writes through the service after the topology change.
    await topology.assert_read_all_keys(service_client, 'add_shard_before_', 'abc')
    await topology.assert_read_all_keys(service_client, 'add_shard_after_', 'cde')
