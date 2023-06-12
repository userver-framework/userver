import asyncio

import pytest

import redis

# Some messages may be lost (it's a Redis limitation)
REQUESTS_RETRIES = 42
REQUESTS_RELAX_TIME = 1.0

# Target channel. Same constant in c++ code
INPUT_CHANNEL_NAME = 'input_channel'


async def _validate_pubsub(redis_db, service_client, msg, queued, redis_type):
    url = f'/redis-{redis_type}'

    for _ in range(REQUESTS_RETRIES):
        redis_db.publish(INPUT_CHANNEL_NAME, msg)

        response = await service_client.get(url, params={'queued': queued})

        assert response.status == 200
        data = response.json()['data']

        if data:
            assert msg in data
            await service_client.delete(url)
            return

        await asyncio.sleep(REQUESTS_RELAX_TIME)

    assert False, 'Retries exceeded'


@pytest.mark.skip(reason='Never worked, fix in TAXICOMMON-6778')
@pytest.mark.parametrize('queued', ['yes', 'no'])
async def test_happy_path_sentinel(service_client, redis_store, queued):
    msg = 'sentinel_message'
    await _validate_pubsub(
        redis_store, service_client, msg, queued, 'sentinel',
    )


@pytest.mark.skip(reason='Flaps, fix in TAXICOMMON-6778')
@pytest.mark.parametrize('queued', ['yes', 'no'])
async def test_happy_path_cluster(service_client, redis_cluster_store, queued):
    async def _test_pubsub(port_number, prefix):
        msg = f'{prefix}_{port_number}'

        with redis.StrictRedis(host='localhost', port=port_number) as db:
            await _validate_pubsub(db, service_client, msg, queued, 'cluster')

    for primary_node in redis_cluster_store.get_primaries():
        await _test_pubsub(primary_node.port, 'primary')

    for replica_node in redis_cluster_store.get_replicas():
        await _test_pubsub(replica_node.port, 'replica')
