import asyncio


FAILOVER_DEADLINE_SEC = 30  # maximum time allowed to finish failover


async def test_happy_path(service_client):
    result = await service_client.post(
        '/redis-sentinel', params={'value': 'abc', 'key': 'key'},
    )
    assert result.status == 201

    result = await service_client.get('/redis-sentinel', params={'key': 'key'})
    assert result.status == 200
    assert result.text == 'abc'


async def test_hard_failover(service_client, redis_sentinel_services):
    # Make a write operation to current master
    result = await service_client.post(
        '/redis-sentinel', params={'key': 'hf_key', 'value': 'abc'},
    )
    assert result.status == 201

    # Ensure that replica does get updates from the master
    result = await service_client.get(
        '/redis-sentinel', params={'key': 'hf_key'},
    )
    assert result.status == 200
    assert result.text == 'abc'

    # Start the failover
    redis_sentinel_services.masters[0].kill()

    # Failover starts in ~10 seconds
    for _ in range(FAILOVER_DEADLINE_SEC):
        result = await service_client.post(
            '/redis-sentinel', params={'key': 'hf_key2', 'value': 'abcd'},
        )
        if result.status == 500:
            await asyncio.sleep(1)
            continue

        break
    assert result.status == 201

    # Now that one of the replicas has become the master,
    # check reading from the remaining replica
    assert (
        await service_client.get('/redis-sentinel', params={'key': 'hf_key'})
    ).text == 'abc'
    assert (
        await service_client.get('/redis-sentinel', params={'key': 'hf_key2'})
    ).text == 'abcd'

    # todo: TAXITOOLS-5772 check that containers are healthy before each test
    redis_sentinel_services.masters[0].start()
    await redis_sentinel_services.masters[0].is_healthy()
