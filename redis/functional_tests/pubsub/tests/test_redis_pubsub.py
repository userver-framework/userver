import asyncio

import redis

# Some messages may be lost (it's a Redis limitation)
REQUESTS_RETRIES = 42
REQUESTS_RELAX_TIME = 1.0

# Target channel. Same constant in c++ code
INPUT_CHANNEL_NAME = 'input_channel'


def _get_url(redis_type):
    return f'/redis-{redis_type}'


async def _validate_pubsub(redis_db, service_client, msg, redis_type):
    url = _get_url(redis_type)
    for _ in range(REQUESTS_RETRIES):
        redis_db.publish(INPUT_CHANNEL_NAME, msg)

        response = await service_client.get(url)

        assert response.status == 200
        data = response.json()['data']

        if data:
            assert msg in data
            await service_client.delete(url)
            return True

        await asyncio.sleep(REQUESTS_RELAX_TIME)

    return False


async def test_happy_path_sentinel(service_client, redis_store):
    msg = 'sentinel_message'
    assert await _validate_pubsub(redis_store, service_client, msg, 'sentinel')


async def test_happy_path_sentinel_with_resubscription(
        service_client, redis_store,
):
    msg = 'sentinel_message'
    response = await service_client.put(_get_url('sentinel'))
    assert response.status == 200
    assert await _validate_pubsub(redis_store, service_client, msg, 'sentinel')


async def test_happy_path_cluster(service_client, redis_cluster_store):
    async def _test_pubsub(port_number, prefix):
        msg = f'{prefix}_{port_number}'

        with redis.StrictRedis(host='localhost', port=port_number) as db:
            return await _validate_pubsub(db, service_client, msg, 'cluster')

    failed = []
    for primary_node in redis_cluster_store.get_primaries():
        instance_ok = await _test_pubsub(primary_node.port, 'primary')
        if not instance_ok:
            failed.append(f'primary {primary_node.port}')

    for replica_node in redis_cluster_store.get_replicas():
        instance_ok = await _test_pubsub(replica_node.port, 'replica')
        if not instance_ok:
            failed.append(f'replica {replica_node.port}')

    if failed:
        assert False, f'Failed after multiple retries: {failed}'
