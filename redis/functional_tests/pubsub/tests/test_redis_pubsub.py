import json

import pytest

import redis


INPUT_CHANNEL_NAME = (
    'input_channel'  # target channel. Same constant in c++ code
)


async def do_test_redis_services_pubsub(
        service_client, redis_services, queued,
):
    async def _test_pubsub(port_number, prefix):
        msg = f'{prefix}_{redis_number}'

        with redis.StrictRedis(
                host='localhost', port=port_number,
        ) as redis_connection:
            redis_connection.publish(INPUT_CHANNEL_NAME, msg)

        result = await service_client.get(
            '/redis-cluster', params={'queued': queued},
        )

        data = sorted(json.loads(result)['data'])

        await service_client.delete('/redis-cluster')

        assert data == [msg]

    for redis_number in range(len(redis_services.masters)):
        _test_pubsub(redis_services.master_port(redis_number), 'master')

    for redis_number in range(len(redis_services.replicas)):
        _test_pubsub(redis_services.replica_port(redis_number), 'replica')


@pytest.mark.parametrize('queued', ['yes', 'no'])
async def test_happy_path_sentinel(
        service_client, redis_sentinel_services, queued,
):
    do_test_redis_services_pubsub(
        service_client, redis_sentinel_services, queued,
    )


@pytest.mark.parametrize('queued', ['yes', 'no'])
async def test_happy_path_cluster(
        service_client, redis_cluster_services, queued,
):
    do_test_redis_services_pubsub(
        service_client, redis_cluster_services, queued,
    )
