import asyncio
import logging

import redis

REDIS_PORT = 6379
KEYS_SEQ_LEN = 10  # enough sequential keys to test all slots
FAILOVER_DEADLINE_SEC = 30  # maximum time allowed to finish failover

logger = logging.getLogger(__name__)


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


async def test_hard_failover(service_client, redis_cluster_services):
    # Write enough different keys to have something in every slot
    assert await _check_write_all_slots(service_client, 'hf_key1', 'abc')

    # Start the failover
    redis_cluster_services.masters[0].kill()

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


def _execute_command(node, cmd):
    ret = node.container.exec_run(cmd.split())
    if ret.exit_code:
        logger.error(
            'Container exec_run failed, cmd = %r, result = %r', cmd, ret,
        )
        raise RuntimeError('Failed to create node')


def _create_node(entry_node, new_node, replica=False):
    entry_node_host = entry_node.get_ipv6()
    new_node_host = new_node.get_ipv6()
    cmd = (
        f'redis-cli --cluster add-node '
        f'{new_node_host}:{REDIS_PORT} '
        f'{entry_node_host}:{REDIS_PORT}'
    )
    if replica:
        cmd += ' --cluster-slave'
    _execute_command(entry_node, cmd)


def _move_hash_slots(from_node, to_node, hash_slot_count):
    from_host = from_node.get_ipv6()
    to_host = to_node.get_ipv6()
    from_client = redis.StrictRedis(host=from_host, port=REDIS_PORT)
    to_client = redis.StrictRedis(host=to_host, port=REDIS_PORT)
    from_id = from_client.cluster('myid').decode()
    to_id = to_client.cluster('myid').decode()
    cmd = (
        f'redis-cli --cluster reshard {from_host}:{REDIS_PORT} '
        f'--cluster-from {from_id} '
        f'--cluster-to {to_id} --cluster-slots {hash_slot_count} '
        f'--cluster-yes'
    )
    _execute_command(from_node, cmd)


async def test_add_shard(service_client, redis_cluster_topology_services):
    # Write enough different keys to have something in every slot
    assert await _check_write_all_slots(service_client, 'hf_key1', 'abc')

    topology = redis_cluster_topology_services

    # Start the failover
    master = topology.masters[0]
    new_master = topology.not_assigned[0]
    new_replica = topology.not_assigned[1]

    # Create empty master node and replica
    _create_node(master, new_master, replica=False)
    topology.wait_cluster_nodes_ready([new_master], 6)

    _create_node(new_master, new_replica, replica=True)
    topology.wait_cluster_nodes_ready([new_replica], 6)
    # wait until UpdateNode
    await asyncio.sleep(10)

    # refard: move 1/4 of slots from each of existing masters to new master
    slot_count = 16384 // 3 // 4
    for master in topology.masters:
        _move_hash_slots(master, new_master, slot_count)

    # wait until new nodes know that they are now storing a hash slots
    topology.wait_cluster_nodes_ready([new_master], 8)
    topology.wait_cluster_nodes_ready([new_replica], 8)
    topology.wait_cluster_nodes_ready(topology.masters, 8)
    topology.wait_cluster_nodes_ready(topology.replicas, 8)

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
