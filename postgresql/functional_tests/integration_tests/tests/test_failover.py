import asyncio


FAILOVER_DEADLINE_SEC = 30  # maximum time allowed to finish failover


async def test_hard_failover(service_client, postgresql_primary_is_healthy):
    response = await service_client.post('/v1/key-value?key=foo&value=bar')
    assert response.status == 201
    assert response.content == b'bar'

    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status == 200
    assert response.content == b'bar'

    postgresql_primary_is_healthy.container.kill()

    for _ in range(FAILOVER_DEADLINE_SEC):
        response = await service_client.get('/v1/key-value?key=foo')
        if response.status == 500:
            await asyncio.sleep(1)
            continue
        break

    assert response.status == 200
