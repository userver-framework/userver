import asyncio
import logging

import pytest
import pytest_userver.utils.sync as sync

logger = logging.getLogger(__name__)

KEYS_SEQ_LEN = 20  # enough sequential keys to test all slots
REDIS_PORT = 6379
FAILOVER_DEADLINE_SEC = 30  # maximum time allowed to finish failover


async def test_happy_path(service_client, redis_cluster_topology):
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
            '/redis-cluster',
            params={'key': f'{key_prefix}{i}'},
        )
        for i in range(KEYS_SEQ_LEN)
    ]
    assert all(res.status == 200 and res.text == value for res in await asyncio.gather(*get_reqs))


async def test_hard_failover(service_client, redis_cluster_topology):
    # Write enough different keys to have something in every slot
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

    # Now that one of the replicas has become the master,
    # check reading from the remaining replica
    await _assert_read_all_slots(service_client, 'hf_key1', 'abc')
    await _assert_read_all_slots(service_client, 'hf_key2', 'cde')


async def _check_modes(
    service_client, db_name, expected_responses_by_modes, max_failed_shards=0, max_failed_shards_percent=0
):
    """
    example of
    expected_responses_by_modes = {
        'no_wait': 'OK',
        'master': 'OK',
        'slave': 'NOT_READY',
        'master_or_slave': 'OK',
        'master_and_slave': 'NOT_READY'
    }
    method should return true if all responses actually equal the expected.
    """
    for mode, expected in expected_responses_by_modes.items():
        response = await service_client.get(
            '/is-ready',
            params={
                'db': db_name,
                'mode': mode,
                'max_failed_shards': max_failed_shards,
                'max_failed_shards_percent': max_failed_shards_percent,
            },
        )
        if response.status != 200 or response.text != expected:
            logger.warning(f'Mode {mode} for {db_name} expected {expected}, got {response.text}')
            return False
    return True


async def test_required_redis_failed_host(service_client, redis_cluster_topology):
    """
    This test shows what happens when one of the cluster's required nodes fails.

    The redis-cluster is not a required Redis cluster; its failure does not cause the service to return 500 errors on pings.
    However, when queried, it may report that some hosts are in a failed state, indicating that it is not ready for certain modes because it does not track a fail flag.

    redis-cluster2 is a required Redis cluster that tracks the fail-flag so it knows not only that the connection is broken, but that the cluster itself is broken.
    This indicates the problem is not on our side, and therefore there is no need to reply with a 500 on ping.
    """

    # waiting for readines
    async def is_ready():
        ping_response = await service_client.get('/ping')
        return (
            ping_response.status_code == 200
            and await _check_modes(
                service_client,
                'redis-cluster',
                {
                    'no_wait': 'OK',
                    'master': 'OK',
                    'slave': 'OK',
                    'master_or_slave': 'OK',
                    'master_and_slave': 'OK',
                },
            )
            and await _check_modes(
                service_client,
                'redis-cluster2',
                {
                    'no_wait': 'OK',
                    'master': 'OK',
                    'slave': 'OK',
                    'master_or_slave': 'OK',
                    'master_and_slave': 'OK',
                },
            )
        )

    node = redis_cluster_topology.get_replicas()[0]
    if node.version < '8.0.0':
        return

    await sync.wait(is_ready, total_wait_seconds=30)

    # Start the failover
    await node.stop()

    async def _check_redis_cluster_modes() -> bool:
        """Check that all required modes for the first Redis cluster are OK."""
        return await _check_modes(
            service_client,
            'redis-cluster',
            {
                'no_wait': 'OK',
                'master': 'OK',
                'slave': 'NOT_READY',
                'master_or_slave': 'OK',
                'master_and_slave': 'NOT_READY',
            },
        )

    async def _check_redis_cluster2_modes() -> bool:
        """
        Check that the required modes for the second Redis cluster match expectations.
        Expect all OKs because it is required.
        Failed states are now taken into account.
        Thus, if a node actually fails rather than simply losing connection, we count it as an OK shard and cluster.
        """
        return await _check_modes(
            service_client,
            'redis-cluster2',
            {
                'no_wait': 'OK',
                'master': 'OK',
                'slave': 'OK',
                'master_or_slave': 'OK',
                'master_and_slave': 'OK',
            },
        )

    # wait till client know that one node is broken
    await sync.wait(_check_redis_cluster_modes, total_wait_seconds=30)

    ping_response = await service_client.get('/ping')
    assert ping_response.status_code == 200

    # this check shoud work because we shutdown on of the nodes so we should know it is broken node so it should be
    # skipped
    await sync.wait(_check_redis_cluster2_modes, total_wait_seconds=30)

    # restart host
    await node.start()
    await sync.wait(is_ready, total_wait_seconds=60)


async def test_required_redis_failed_connection(service_client, redis_cluster_topology):
    """
    This test demonstrates what happens when the service fails to connect to one of the cluster's nodes.

    The redis-cluster is not a required Redis cluster; its failure does not cause the service to return 500 errors on pings.
    However, when queried, it may report that some hosts are in a failed state, indicating that it is not ready for certain modes because it does not track a fail flag.

    redis-cluster2 is a required Redis cluster that tracks the fail flag, so it knows not only that the connection is broken,
    but also that the cluster itself is not broken—there is no fail flag on the host with the broken connection.
    This indicates the problem is on our side, and therefore it needs to reply with a 500 on ping.
    """

    # waiting for readines
    async def is_ready():
        ping_response = await service_client.get('/ping')
        return (
            ping_response.status_code == 200
            and await _check_modes(
                service_client,
                'redis-cluster',
                {
                    'no_wait': 'OK',
                    'master': 'OK',
                    'slave': 'OK',
                    'master_or_slave': 'OK',
                    'master_and_slave': 'OK',
                },
            )
            and await _check_modes(
                service_client,
                'redis-cluster2',
                {
                    'no_wait': 'OK',
                    'master': 'OK',
                    'slave': 'OK',
                    'master_or_slave': 'OK',
                    'master_and_slave': 'OK',
                },
            )
        )

    node = redis_cluster_topology.get_replicas()[0]
    if node.version < '8.0.0':
        return

    await sync.wait(is_ready, total_wait_seconds=30)

    # Break connection to node
    await node.proxy.stop()

    # After a required Redis host becomes unreachable, ping should return 500
    async def _ping_fails() -> bool:
        """Return True if the /ping endpoint responds with HTTP 500."""
        ping_response = await service_client.get('/ping')
        return ping_response.status_code == 500

    async def _check_redis_cluster_modes() -> bool:
        """Check that all required modes for the first Redis cluster are OK."""
        return await _check_modes(
            service_client,
            'redis-cluster',
            {
                'no_wait': 'OK',
                'master': 'OK',
                'slave': 'NOT_READY',
                'master_or_slave': 'OK',
                'master_and_slave': 'NOT_READY',
            },
        )

    async def _check_redis_cluster2_modes() -> bool:
        """
        Verify that the required modes for the second Redis cluster match expectations.
        Failed states are now taken into account;
        if a node is not in a failed state but the connection is broken, it is counted as NOT_READY.
        """
        return await _check_modes(
            service_client,
            'redis-cluster2',
            {
                'no_wait': 'OK',
                'master': 'OK',
                'slave': 'NOT_READY',
                'master_or_slave': 'OK',
                'master_and_slave': 'NOT_READY',
            },
        )

    async def _check_redis_cluster2_modes_max_failed_shards() -> bool:
        """
        Verify that the required modes for the second Redis cluster match expectations.
        Failed states are now taken into account;
        if a node is not in a failed state but the connection is broken, it is counted as NOT_READY.
        However, since max_failed_shards=1, we tolerate the single failed shard and still count it as OK.
        """
        return await _check_modes(
            service_client,
            'redis-cluster2',
            {
                'no_wait': 'OK',
                'master': 'OK',
                'slave': 'OK',
                'master_or_slave': 'OK',
                'master_and_slave': 'OK',
            },
            max_failed_shards=1,
        )

    async def _check_redis_cluster2_modes_max_failed_shards_percent() -> bool:
        """
        Verify that the required modes for the second Redis cluster match expectations.
        Failed states are now taken into account;
        if a node is not in a failed state but the connection is broken, it is counted as NOT_READY.
        However, since max_failed_shards_percent=34, we tolerate the single failed shard (33%) and still count it as OK.
        """
        return await _check_modes(
            service_client,
            'redis-cluster2',
            {
                'no_wait': 'OK',
                'master': 'OK',
                'slave': 'OK',
                'master_or_slave': 'OK',
                'master_and_slave': 'OK',
            },
            max_failed_shards_percent=34,
        )

    await sync.wait(_ping_fails, total_wait_seconds=40)

    # wait till client know that one node is broken
    await sync.wait(_check_redis_cluster_modes, total_wait_seconds=30)

    # This differs from test_required_redis_failed_host.
    # Since we do not have a fail flag on the node we are having connection problems with, the issue is on our side.
    # Therefore, we should return 500 on ping.
    await sync.wait(_check_redis_cluster2_modes, total_wait_seconds=30)

    await sync.wait(_check_redis_cluster2_modes_max_failed_shards, total_wait_seconds=30)
    await sync.wait(_check_redis_cluster2_modes_max_failed_shards_percent, total_wait_seconds=30)

    # restart the connection
    node.proxy.start()
    await sync.wait(is_ready, total_wait_seconds=60)


@pytest.mark.skip(reason='Flaky TAXICOMMON-11677')
async def test_add_shard(service_client, redis_cluster_topology):
    # Write enough different keys to have something in every slot
    assert await _check_write_all_slots(service_client, 'hf_key1', 'abc')

    await redis_cluster_topology.add_shard()

    # Failover starts in ~10 seconds
    async def is_ready():
        return await _check_write_all_slots(
            service_client,
            'hf_key2',
            'cde',
        )

    await sync.wait(is_ready)

    # Now that one of the replicas has become the master,
    # check reading from the remaining replica
    await _assert_read_all_slots(service_client, 'hf_key1', 'abc')
    await _assert_read_all_slots(service_client, 'hf_key2', 'cde')
