import pytest
import pytest_userver.utils.sync as sync
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

    async def check_ready():
        redis_db.publish(INPUT_CHANNEL_NAME, msg)

        response = await service_client.get(url)

        assert response.status == 200
        data = response.json()['data']

        if data:
            assert msg in data
            await service_client.delete(url)
            return True

        raise sync.NotReady()

    try:
        await sync.wait_until(check_ready)
        return True
    except AssertionError:
        return False


@pytest.mark.parametrize('db_name', ['sentinel', 'sentinel-with-master'])
async def test_happy_path_sentinel(service_client, redis_store, db_name):
    msg = 'sentinel_message'
    assert await _validate_pubsub(redis_store, service_client, msg, db_name)


@pytest.mark.parametrize('db_name', ['sentinel', 'sentinel-with-master'])
async def test_happy_path_sentinel_with_resubscription(
    service_client,
    redis_store,
    db_name,
):
    msg = 'sentinel_message'
    response = await service_client.put(_get_url(db_name))
    assert response.status == 200
    assert await _validate_pubsub(redis_store, service_client, msg, db_name)


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


@pytest.mark.parametrize('redis_group_mode', ['paired'], indirect=True)
@pytest.mark.uservice_oneshot(config_hooks=['empty_cluster_shards_config'])
async def test_paired_cluster_with_empty_shards(service_client, redis_cluster_store):
    assert await _validate_pubsub(redis_cluster_store, service_client, 'empty_shards_message', 'cluster')
    response = await service_client.get('/redis-cluster', params={'check-command-client': 'true'})
    assert response.status == 200
    assert response.json()['command_client_available'] is False


async def test_happy_path_standalone(service_client, redis_standalone_store):
    msg = 'sentinel_message'
    assert await _validate_pubsub(redis_standalone_store, service_client, msg, 'standalone')


async def test_happy_path_standalone_with_resubscription(service_client, redis_standalone_store):
    msg = 'sentinel_message'
    response = await service_client.put(_get_url('standalone'))
    assert response.status == 200
    assert await _validate_pubsub(redis_standalone_store, service_client, msg, 'standalone')


@pytest.mark.parametrize('redis_type', ['sentinel', 'standalone', 'cluster'])
async def test_publisher(
    service_client, redis_store, redis_standalone_store, redis_cluster_store, redis_type, redis_group_mode
):
    store = {'sentinel': redis_store, 'standalone': redis_standalone_store, 'cluster': redis_cluster_store}[redis_type]
    assert await _validate_pubsub(store, service_client, 'ready', redis_type)
    p = store.pubsub()
    p.subscribe('output-channel')
    path = '/publisher' if redis_type == 'sentinel' else f'/{redis_type}-publisher'
    response = await service_client.get(f'{path}?message=hello')
    assert response.status == 200
    assert response.json() == {'value': 'hello', 'shared_sentinel': redis_group_mode != 'separate'}

    def get_message(p):
        for i in range(15):
            message = p.get_message(timeout=1.0)
            if message and message['type'] == 'message':
                return message['data'].decode()
        return None

    assert get_message(p) == 'hello'
