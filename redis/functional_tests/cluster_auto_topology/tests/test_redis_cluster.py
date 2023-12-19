import asyncio
import logging
import time

KEYS_SEQ_LEN = 20  # enough sequential keys to test all slots
REDIS_PORT = 6379
FAILOVER_DEADLINE_SEC = 30  # maximum time allowed to finish failover

logger = logging.getLogger(__name__)


async def test_happy_path(service_client, redis_cluster_topology):
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


async def _check_write_all_slots(service_client, key_prefix, value):
    post_reqs = [
        service_client.post(
            '/redis-cluster',
            params={'key': f'{key_prefix}{i}', 'value': value},
        )
        for i in range(KEYS_SEQ_LEN)
    ]
    return all(res.status != 500 for res in await asyncio.gather(*post_reqs))


async def _assert_read_all_slots(service_client, key_prefix, value):
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


async def test_hard_failover(service_client, redis_cluster_topology):
    # Write enough different keys to have something in every slot
    assert await _check_write_all_slots(service_client, 'hf_key1', 'abc')

    # Start the failover
    redis_cluster_topology.get_masters()[0].stop()
    # wait until service detect that shard 0 is broken
    time.sleep(6)

    # Failover starts in ~10 seconds
    for _ in range(FAILOVER_DEADLINE_SEC):
        write_ok = await _check_write_all_slots(
            service_client, 'hf_key2', 'cde',
        )
        if not write_ok:
            await asyncio.sleep(1)
            continue
        break
    assert write_ok

    # Now that one of the replicas has become the master,
    # check reading from the remaining replica
    await _assert_read_all_slots(service_client, 'hf_key1', 'abc')
    await _assert_read_all_slots(service_client, 'hf_key2', 'cde')


async def test_add_shard(service_client, redis_cluster_topology):
    # Write enough different keys to have something in every slot
    assert await _check_write_all_slots(service_client, 'hf_key1', 'abc')

    redis_cluster_topology.add_shard()

    # Failover starts in ~10 seconds
    for _ in range(FAILOVER_DEADLINE_SEC):
        write_ok = await _check_write_all_slots(
            service_client, 'hf_key2', 'cde',
        )
        if not write_ok:
            await asyncio.sleep(1)
            continue

        break
    assert write_ok

    # Now that one of the replicas has become the master,
    # check reading from the remaining replica
    await _assert_read_all_slots(service_client, 'hf_key1', 'abc')
    await _assert_read_all_slots(service_client, 'hf_key2', 'cde')


async def test_cluster_switcher(
        service_client, redis_cluster_topology, dynamic_config,
):
    """
    Check service reacts correctly on dynamic config change.
    """
    # Write enough different keys to have something in every slot
    assert await _check_write_all_slots(service_client, 'hf_key1', 'abc')

    # test we can turn autotopology off
    dynamic_config.set_values({'REDIS_CLUSTER_AUTOTOPOLOGY_ENABLED_V2': False})
    await asyncio.sleep(10)
    await _assert_read_all_slots(service_client, 'hf_key1', 'abc')

    # test we can turn autotopology on again
    dynamic_config.set_values({'REDIS_CLUSTER_AUTOTOPOLOGY_ENABLED_V2': True})
    await asyncio.sleep(10)
    await _assert_read_all_slots(service_client, 'hf_key1', 'abc')
    redis_cluster_topology.add_shard()
    await _assert_read_all_slots(service_client, 'hf_key1', 'abc')
