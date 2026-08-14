async def _insert_row(service_client, row):
    return await service_client.post('ydb/insert-row', json=row)


async def test_insert_row(service_client, ydb):
    response = await _insert_row(
        service_client,
        {
            'id': 'id-insert',
            'name': 'name-insert',
            'service': 'srv',
            'channel': 123,
        },
    )
    assert response.status_code == 200
    assert response.json() == {}

    cursor = ydb.execute('SELECT * FROM events WHERE id = "id-insert"')
    assert len(cursor) == 1
    assert len(cursor[0].rows) == 1


async def test_insert_row_duplicate_pk_conflict(service_client):
    row = {
        'id': 'id-insert-duplicate',
        'name': 'name-insert-duplicate',
        'service': 'srv',
        'channel': 123,
    }

    response = await _insert_row(service_client, row)
    assert response.status_code == 200

    response = await _insert_row(service_client, row)
    assert response.status_code == 409
