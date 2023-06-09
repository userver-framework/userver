import asyncio

import redis


KEYS_SEQ_LEN = 10  # enough sequential keys to test all shards
FAILOVER_DEADLINE_SEC = 30  # maximum time allowed to finish failover


async def test_happy_path(service_client):
    post_reqs = [
        service_client.post(
            '/redis-cluster', params={'key': f'key{i}', 'value': 'abc'},
        )
        for i in range(KEYS_SEQ_LEN)
    ]
    assert all(res.status == 201 for res in await asyncio.gather(*post_reqs))

    get_reqs = [
        service_client.get('/redis-cluster', params={'key': f'key{i}'})
        for i in range(KEYS_SEQ_LEN)
    ]
    assert all(
        res.status == 200 and res.text == 'abc'
        for res in await asyncio.gather(*get_reqs)
    )


async def _check_write_all_shards(service_client, key_prefix, value):
    post_reqs = [
        service_client.post(
            '/redis-cluster',
            params={'key': f'{key_prefix}{i}', 'value': value},
        )
        for i in range(KEYS_SEQ_LEN)
    ]
    return all(res.status != 500 for res in await asyncio.gather(*post_reqs))


async def _assert_read_all_shards(service_client, key_prefix, value):
    get_reqs = [
        service_client.get(
            '/redis-cluster', params={'key': f'{key_prefix}{i}'},
        )
        for i in range(KEYS_SEQ_LEN)
    ]
    assert all(
        res.status == 200 and res.text == value
        for res in await asyncio.gather(*get_reqs)
    )


async def _assert_failover_completed(service_client, key_prefix, value):
    for _ in range(FAILOVER_DEADLINE_SEC):
        write_ok = await _check_write_all_shards(
            service_client, key_prefix, value,
        )
        if write_ok:
            break
        await asyncio.sleep(1)
    assert write_ok


async def test_failover(service_client, redis_cluster_store):
    # Write enough different keys to have something in every shard
    assert await _check_write_all_shards(service_client, 'hf_key1', 'abc')

    # Select primary-replica pair and start the failover
    primary = redis_cluster_store.get_node_from_key('key', replica=False)
    replica = redis_cluster_store.get_node_from_key('key', replica=True)
    assert redis_cluster_store.cluster_failover(target_node=replica)

    # await _assert_failover_completed(service_client, 'hf_key2', 'cde')
    for _ in range(FAILOVER_DEADLINE_SEC):
        write_ok = await _check_write_all_shards(
            service_client, 'hf_key2', 'cde',
        )
        if write_ok:
            break
        await asyncio.sleep(1)
    assert write_ok

    # Now that one of the replicas has become the master,
    # check reading from the remaining replica
    await _assert_read_all_shards(service_client, 'hf_key1', 'abc')
    await _assert_read_all_shards(service_client, 'hf_key2', 'cde')

    # Failover master back where it was and make sure it gets there
    assert redis_cluster_store.cluster_failover(target_node=primary)
    for _ in range(FAILOVER_DEADLINE_SEC):
        try:
            redis_cluster_store.set('key', 'value')
            break
        except redis.exceptions.ReadOnlyError:
            await asyncio.sleep(1)
