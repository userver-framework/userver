import asyncio

import redis


# Some messages may be lost (it's a Redis limitation)
REDIS_PORT = 6379
REQUESTS_RETRIES = 42
REQUESTS_RETRIES_NOT_EXPECTED = 3
REQUESTS_RELAX_TIME = 1.0

# Target channel. Same constant in c++ code
INPUT_CHANNEL_PREFIX = 'input_channel@'
SHARDED_INPUT_CHANNEL_PREFIX = 'input_sharded_channel@'
INPUT_CHANNELS_COUNT = 5
OUTPUT_CHANNEL_NAME = 'output_channel'
OUTPUT_SCHANNELS_COUNT = 10


def _publish_callback(redis_db, channel, message):
    redis_db.publish(channel, message)


def _spublish_callback(redis_db, channel, message):
    redis_db.spublish(channel, message)


async def _validate_pubsub(
        redis_db, service_client, channel_prefix, msg, publish_method,
):
    """
    publish to redis_db and expect data accessible in service via handler
    """
    url = '/redis-cluster'

    for i in range(INPUT_CHANNELS_COUNT):
        channel_name = channel_prefix + str(i)
        message = msg + str(i)
        for _ in range(REQUESTS_RETRIES):
            publish_method(redis_db, channel_name, message)

            response = await service_client.get(
                url, params={'read': channel_name},
            )

            assert response.status == 200
            data = response.json()['data']

            if data:
                assert message in data
                await service_client.delete(url)
                break

            await asyncio.sleep(REQUESTS_RELAX_TIME)
        else:
            assert (
                False
            ), f'Retries exceeded trying to read from {channel_name}'


async def _test_service_subscription(service_client, node, prefix):
    msg = f'{prefix}_{node.get_address()}'
    db = node.get_client()
    await _validate_pubsub(
        db, service_client, INPUT_CHANNEL_PREFIX, msg, _publish_callback,
    )


async def _test_service_sharded_subscription(
        service_client, cluster_client, prefix,
):
    msg = f'{prefix}'
    await _validate_pubsub(
        cluster_client,
        service_client,
        SHARDED_INPUT_CHANNEL_PREFIX,
        msg,
        _spublish_callback,
    )


async def _validate_service_publish(service_client, nodes, shards_range):
    """
    Check publishing by service with 'publish' command in each shard is heard
    from any node from any shard
    """
    redis_clients = [(node, node.get_client()) for node in nodes]

    async def _get_message(pubsub, retries=5, delay=0.5):
        ret = None
        for _ in range(retries):
            ret = pubsub.get_message()
            if ret is not None:
                return ret
            await asyncio.sleep(delay)
        return ret

    async def _ensure_published(
            pubsub, expected_message, retries=5, delay=0.5,
    ):
        for _ in range(retries):
            ret = pubsub.get_message()
            if (
                    ret is not None
                    and ret['type'] == 'message'
                    and ret['channel'] == OUTPUT_CHANNEL_NAME.encode()
                    and ret['data'] == expected_message
            ):
                return
            await asyncio.sleep(delay)
        assert False, 'Retries exceeded'

    async def _validate(service_client, pubsub, msg, shard):
        for _ in range(REQUESTS_RETRIES):
            try:
                response = await service_client.get(
                    url, params={'publish': msg, 'shard': shard},
                )
                assert response.status == 200

                await _ensure_published(pubsub, msg.encode())
                return
            except Exception as exc:  # pylint: disable=broad-except
                print(f'Pubsub validation failed for shard {shard}: {exc}')
            await asyncio.sleep(REQUESTS_RELAX_TIME)
        assert False, f'Retries exceeded: shard={shard}'

    for node, redis_client in redis_clients:
        pubsub = redis_client.pubsub()
        pubsub.subscribe(OUTPUT_CHANNEL_NAME)
        subscribe_message = await _get_message(
            pubsub, retries=REQUESTS_RETRIES,
        )
        assert subscribe_message['type'] == 'subscribe'

        url = '/redis-cluster'
        if shards_range:
            for shard in shards_range:
                msg = f'test_{shard}_{node.get_address()}'
                await _validate(service_client, pubsub, msg, shard)
        else:
            msg = f'test_nonshard_{node.get_address()}'
            await _validate(service_client, pubsub, msg, '')


async def _validate_service_spublish(service_client, nodes):
    """
    Check publishing by service with 'spublish' command in each shard is heard
    by redis
    """

    async def _get_message(pubsub, retries=5, delay=0.5):
        ret = None
        for _ in range(retries):
            ret = pubsub.get_sharded_message()
            if ret is not None:
                return ret
            await asyncio.sleep(delay)
        return ret

    async def _ensure_published(
            pubsub, channel, expected_message, retries=5, delay=0.5,
    ):
        for _ in range(retries):
            ret = pubsub.get_sharded_message()
            if (
                    ret is not None
                    and ret['type'] == 'smessage'
                    and ret['channel'] == channel
                    and ret['data'] == expected_message
            ):
                return
            await asyncio.sleep(delay)
        assert False, 'Retries exceeded'

    async def _validate(service_client, pubsub, msg, channel_name):
        for _ in range(REQUESTS_RETRIES):
            try:
                response = await service_client.get(
                    '/redis-cluster',
                    params={'spublish': msg, 'channel': channel_name},
                )
                assert response.status == 200

                await _ensure_published(
                    pubsub, channel_name.encode(), msg.encode(),
                )
                return
            except Exception as exc:  # pylint: disable=broad-except
                print(f'Spubsub validation failed for {channel_name}: {exc}')
            await asyncio.sleep(REQUESTS_RELAX_TIME)
        assert False, f'Retries exceeded: shard={channel_name}'

    # create redis cluster client
    for node in nodes:
        try:
            redis_cluster = redis.RedisCluster(host=node.host, port=node.port)
        except redis.exceptions.RedisClusterException:
            pass

    for i in range(OUTPUT_SCHANNELS_COUNT):
        channel_name = OUTPUT_CHANNEL_NAME + str(i)
        channel_name_bytes = channel_name.encode()
        pubsub = redis_cluster.pubsub()
        pubsub.ssubscribe(channel_name)
        subscribed_msg = await _get_message(pubsub)
        assert subscribed_msg == {
            'type': 'ssubscribe',
            'pattern': None,
            'channel': channel_name_bytes,
            'data': 1,
        }
        await _validate(service_client, pubsub, channel_name, channel_name)


async def _check_shard_count(service_client, expected_shard_count):
    """
    Check redis_cluster_client knows how many shard has redis cluster
    """
    expected = f'{expected_shard_count}'.encode()
    for _ in range(REQUESTS_RETRIES):
        try:
            response = await service_client.get(
                '/redis-cluster', params={'shard_count': 'aaa'},
            )
            assert response.status == 200
            if response.content == expected:
                return
        except Exception as exc:  # pylint: disable=broad-except
            print(f'Exception in _check_shard_count: {exc}')
        await asyncio.sleep(REQUESTS_RELAX_TIME)
    assert False, 'Retries exceeded'


def _get_alive_shards_by_service(master_nodes):
    """
    Get alive shard numbers (service numeration)
    """
    # find first alive node
    alive_node = None
    for node in master_nodes:
        if node.started:
            alive_node = node
            break
    client = alive_node.get_client()

    # find shard nubers for master as it is in service
    slots = client.cluster('slots')
    masters_shards = dict()
    i = 0
    for _, _, master, _ in slots:
        master_addr = f'{master[0].decode()}:{master[1]}'
        if master_addr not in masters_shards:
            masters_shards[master_addr] = i
            i += 1
    return [
        masters_shards[x.get_address()]
        for x in master_nodes
        if x.started and x.get_address() in masters_shards
    ]


async def test_cluster_happy_path(service_client, redis_cluster_topology):
    """
    Check service can receive messages published on any of cluster's node
    """

    await service_client.delete('/redis-cluster')

    # test initial config
    for node in redis_cluster_topology.nodes:
        await _test_service_subscription(service_client, node, 'initial_')

    await _check_shard_count(service_client, 3)
    await _validate_service_publish(
        service_client, redis_cluster_topology.nodes, range(3),
    )
    await _validate_service_spublish(
        service_client, redis_cluster_topology.nodes,
    )
    await _test_service_sharded_subscription(
        service_client, redis_cluster_topology.get_client(), 'ssubscription_',
    )


async def test_cluster_add_shard(service_client, redis_cluster_topology):
    """
    Check service still can receive messages published on any of cluster's node
    even after addition of new shard
    """

    await service_client.delete('/redis-cluster')

    # Add shard
    await redis_cluster_topology.add_shard()

    for node in redis_cluster_topology.nodes:
        await _test_service_subscription(service_client, node, 'extended_')

    await _check_shard_count(service_client, 4)
    await _validate_service_publish(
        service_client, redis_cluster_topology.nodes, range(4),
    )
    await _test_service_sharded_subscription(
        service_client,
        redis_cluster_topology.get_client(),
        'ssubscription_extended_',
    )

    # restore initial config
    await redis_cluster_topology.remove_shard()
    await _check_shard_count(service_client, 3)

    for node in redis_cluster_topology.nodes:
        await _test_service_subscription(service_client, node, 'restored_')

    await _validate_service_publish(
        service_client, redis_cluster_topology.nodes, range(3),
    )
    await _test_service_sharded_subscription(
        service_client,
        redis_cluster_topology.get_client(),
        'ssubscription_restored_',
    )


# kill first three shards and leave only fourth shard
async def test_cluster_failover_pubsub(service_client, redis_cluster_topology):
    await service_client.delete('/redis-cluster')

    original_masters = redis_cluster_topology.get_masters()
    original_replicas = redis_cluster_topology.get_replicas()
    original_nodes = original_masters + original_replicas

    await _check_shard_count(service_client, 3)

    # Add shard
    await redis_cluster_topology.add_shard()
    await _check_shard_count(service_client, 4)

    # kill all nodes from first three shards
    for node in original_nodes:
        node.stop()

    # should work due to ClusterSintinelImpl (Publish)
    new_nodes = [
        redis_cluster_topology.added_master,
        redis_cluster_topology.added_replica,
    ]

    await _validate_service_publish(service_client, new_nodes, None)

    # should work due to ClusterSubscriptionStorage (Subscribe)
    for node in new_nodes:
        await _test_service_subscription(
            service_client, node, 'the_last_shard',
        )


# kill all replicas
async def test_cluster_failover_pubsub2(
        service_client, redis_cluster_topology,
):
    """
    killing replicas do not trigger cluster update (topology does not change)
    """
    await service_client.delete('/redis-cluster')

    masters = redis_cluster_topology.get_masters()
    replicas = redis_cluster_topology.get_replicas()

    await _check_shard_count(service_client, 3)

    # kill all replicas
    for node in replicas:
        node.stop()

    await _validate_service_publish(service_client, masters, None)
    await _validate_service_publish(service_client, masters, range(3))

    # should work due to ClusterSubscriptionStorage
    for node in masters:
        await _test_service_subscription(service_client, node, 'only_masters_')
    await _test_service_sharded_subscription(
        service_client,
        redis_cluster_topology.get_client(),
        'ssubscription_only_masters_',
    )
