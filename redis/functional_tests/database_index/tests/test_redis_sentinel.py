DATABASES_COUNT = 3
KEYS_SEQ_LEN = 10  # enough sequential keys to test all shards


async def test_databases_isolation(service_client):
    for database_index in range(DATABASES_COUNT):
        for key_index in range(KEYS_SEQ_LEN):
            result = await service_client.post(
                f'/redis-sentinel-{database_index}',
                params={'value': f'sentinel-{database_index}', 'key': f'key-{key_index}'},
            )
            assert result.status == 201

    for database_index in range(DATABASES_COUNT):
        for key_index in range(KEYS_SEQ_LEN):
            result = await service_client.get(f'/redis-sentinel-{database_index}', params={'key': f'key-{key_index}'})
            assert result.status == 200
            assert result.text == f'sentinel-{database_index}'
