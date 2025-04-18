import asyncio

import pytest
import redis

KEYS_SEQ_LEN = 10  # enough sequential keys to test all shards
FAILOVER_DEADLINE_SEC = 30  # maximum time allowed to finish failover


async def test_happy_path(service_client):
    post_reqs = [
        service_client.post(
            '/redis-cluster',
            params={'key': f'key{i}', 'value': 'abc'},
        )
        for i in range(KEYS_SEQ_LEN)
    ]
    assert all(res.status == 201 for res in await asyncio.gather(*post_reqs))

    get_reqs = [service_client.get('/redis-cluster', params={'key': f'key{i}'}) for i in range(KEYS_SEQ_LEN)]
    assert all(res.status == 200 and res.text == 'abc' for res in await asyncio.gather(*get_reqs))


async def _check_write_all_shards(service_client, key_prefix, value):
    post_reqs = [
        service_client.post(
            '/redis-cluster',
            params={'key': f'{key_prefix}{i}', 'value': value},
        )
        for i in range(KEYS_SEQ_LEN)
    ]
    return all(res.status != 500 for res in await asyncio.gather(*post_reqs))


async def _check_read_all_shards(service_client, key_prefix, value):
    get_reqs = [
        service_client.get(
            '/redis-cluster',
            params={'key': f'{key_prefix}{i}'},
        )
        for i in range(KEYS_SEQ_LEN)
    ]
    return all(res.status == 200 and res.text == value for res in await asyncio.gather(*get_reqs))


async def _assert_read_all_shards(service_client, key_prefix, value):
    assert await _check_read_all_shards(service_client, key_prefix, value)


@pytest.mark.skip(reason='Flaps, fix in TAXICOMMON-8282')
async def test_failover(service_client, redis_cluster_store):
    # Write enough different keys to have something in every shard
    assert await _check_write_all_shards(service_client, 'hf_key1', 'abc')
    assert redis_cluster_store.get_replicas(), 'No replicas in cluster'

    # Select primary-replica pair and start the failover
    primary = redis_cluster_store.get_node_from_key('key', replica=False)
    replica = redis_cluster_store.get_node_from_key('key', replica=True)

    assert primary, 'No primary for node'
    assert replica, 'No replica for node'
    assert redis_cluster_store.cluster_failover(target_node=replica)

    for _ in range(FAILOVER_DEADLINE_SEC):
        write_ok = await _check_write_all_shards(service_client, 'hf_key2', 'cde')
        if write_ok:
            break
        await asyncio.sleep(1)
    assert write_ok

    # Now that one of the replicas has become the master,
    # check reading from the remaining replica
    await _assert_read_all_shards(service_client, 'hf_key1', 'abc')

    # Replica may be syncing, use some retries
    for _ in range(FAILOVER_DEADLINE_SEC):
        read_ok = await _check_read_all_shards(service_client, 'hf_key2', 'cde')
        if read_ok:
            break
        await asyncio.sleep(1)
    assert read_ok

    # Failover master back where it was and make sure it gets there
    assert redis_cluster_store.cluster_failover(target_node=primary)
    for _ in range(FAILOVER_DEADLINE_SEC):
        try:
            redis_cluster_store.set('key', 'value')
            break
        except redis.exceptions.ReadOnlyError:
            await asyncio.sleep(1)
