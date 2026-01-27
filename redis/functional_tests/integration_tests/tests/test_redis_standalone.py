import asyncio

KEYS_SEQ_LEN = 10  # enough sequential keys to test all shards
FAILOVER_DEADLINE_SEC = 30  # maximum time allowed to finish failover
URL_PATH = '/redis-standalone'


async def test_happy_path(service_client):
    post_reqs = [
        service_client.post(
            URL_PATH,
            params={'key': f'key{i}', 'value': 'abc'},
        )
        for i in range(KEYS_SEQ_LEN)
    ]
    assert all(res.status == 201 for res in await asyncio.gather(*post_reqs))

    get_reqs = [service_client.get(URL_PATH, params={'key': f'key{i}'}) for i in range(KEYS_SEQ_LEN)]
    assert all(res.status == 200 and res.text == 'abc' for res in await asyncio.gather(*get_reqs))
