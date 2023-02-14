import asyncio


KEYS_SEQ_LEN = 10  # enough sequential keys to test all slots
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
        write_ok = _check_write_all_slots(service_client, 'hf_key2', 'cde')
        if not write_ok:
            await asyncio.sleep(1)
            continue

        break
    assert write_ok

    # Now that one of the replicas has become the master,
    # check reading from the remaining replica
    assert _assert_read_all_slots(service_client, 'hf_key1', 'abc')
    assert _assert_read_all_slots(service_client, 'hf_key2', 'cde')
