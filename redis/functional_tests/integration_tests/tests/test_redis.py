import asyncio

import redis


async def test_happy_path(_redis_service_settings, service_client):
    result = await service_client.post(
        '/redis', params={'value': 'abc', 'key': 'key'},
    )
    assert result.status == 201

    result = await service_client.get('/redis', params={'key': 'key'})
    assert result.status == 200
    assert result.text == 'abc'


async def test_hard_failover(
        _redis_service_settings,
        service_client,
        redis_docker_service,
        redis_master_0_docker_service,
):
    result = await service_client.post(
        '/redis', params={'value': 'abc', 'key': 'hf_key'},
    )
    assert result.status == 201

    red = redis.StrictRedis(
        host=_redis_service_settings.host,
        port=_redis_service_settings.slave_ports[0],
    )
    # check that the slave does get updates from the master
    response = red.execute_command('get', 'hf_key')
    assert response == b'abc'

    # Start the failover
    redis_master_0_docker_service.kill()

    # Failover starts in ~15 seconds
    for _ in range(20):
        try:
            response = red.execute_command('set', 'hf_key', 'newvalue')
            assert response is True
            break
        except redis.exceptions.ReadOnlyError:
            await asyncio.sleep(1)
            continue
    response = red.execute_command('set', 'hf_key', 'newvalue')
    assert response is True

    # write is OK
    for _ in range(20):
        result = await service_client.post(
            '/redis', params={'value': 'abcd', 'key': 'hf_key2'},
        )
        if result.status == 500:
            continue

        break
    assert result.status == 201

    # read is OK
    result = await service_client.get('/redis', params={'key': 'hf_key2'})
    assert result.status == 200
    assert result.text == 'abcd'
