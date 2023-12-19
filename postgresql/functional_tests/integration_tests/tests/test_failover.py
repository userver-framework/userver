import asyncio

import pytest

import taxi.integration_testing as it


FAILOVER_DEADLINE_SEC = 30  # maximum time allowed to finish failover


@pytest.mark.skip(reason='Flacky fix in TAXICOMMON-6756')
async def test_hard_failover(service_client, testenv: it.Environment):
    response = await service_client.post('/v1/key-value?key=foo&value=bar')
    assert response.status == 201
    assert response.content == b'bar'

    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status == 200
    assert response.content == b'bar'

    await asyncio.sleep(30)
    testenv.pgsql_primary_container.kill()

    for _ in range(FAILOVER_DEADLINE_SEC):
        response = await service_client.get('/v1/key-value?key=foo')
        if response.status == 500:
            await asyncio.sleep(1)
            continue
        break

    assert response.status == 200
