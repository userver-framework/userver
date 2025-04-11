import asyncio

KEYS_SEQ_LEN = 10  # enough sequential keys to test all shards
FAILOVER_DEADLINE_SEC = 30  # maximum time allowed to finish failover


async def test_happy_path(service_client):
    result = await service_client.post(
        '/redis-sentinel',
        params={'value': 'abc', 'key': 'key'},
    )
    assert result.status == 201

    result = await service_client.get('/redis-sentinel', params={'key': 'key'})
    assert result.status == 200
    assert result.text == 'abc'


async def _check_write_all_shards(service_client, key_prefix, value):
    post_reqs = [
        service_client.post(
            '/redis-sentinel',
            params={'key': f'{key_prefix}{i}', 'value': value},
        )
        for i in range(KEYS_SEQ_LEN)
    ]
    return all(res.status != 500 for res in await asyncio.gather(*post_reqs))


async def test_failover(service_client, redis_sentinel):
    # Make a write operation to current master
    result = await service_client.post(
        '/redis-sentinel',
        params={'key': 'hf_key', 'value': 'abc'},
    )
    assert result.status == 201

    # Ensure that replica does get updates from the master
    result = await service_client.get(
        '/redis-sentinel',
        params={'key': 'hf_key'},
    )
    assert result.status == 200
    assert result.text == 'abc'

    # Start the failover
    redis_sentinel.sentinel_failover('test_master1')

    # Wait for failover to happen
    for i in range(FAILOVER_DEADLINE_SEC):
        write_ok = await _check_write_all_shards(service_client, 'failover_{i}_', 'xyz')
        if write_ok:
            break
        await asyncio.sleep(1)
    assert write_ok

    # Now that one of the replicas has become the master,
    # check reading from the remaining replica
    assert (await service_client.get('/redis-sentinel', params={'key': 'hf_key'})).text == 'abc'

    result = await service_client.post(
        '/redis-sentinel',
        params={'key': 'hf_key2', 'value': 'abcd'},
    )
    assert result.status == 201
    assert (await service_client.get('/redis-sentinel', params={'key': 'hf_key2'})).text == 'abcd'
