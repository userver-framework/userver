async def test_upsert_row(service_client, ydb):
    response = await service_client.post(
        'ydb/upsert-row',
        json={
            'id': 'id-upsert',
            'name': 'name-upsert',
            'service': 'srv',
            'channel': 123,
        },
    )
    assert response.status_code == 200
    assert response.json() == {}

    cursor = ydb.execute('SELECT * FROM events WHERE id = "id-upsert"')
    assert len(cursor) == 1
    assert len(cursor[0].rows) == 1

    row = cursor[0].rows[0]
    assert row.pop('created') > 0
    assert row == {
        'id': b'id-upsert',
        'name': 'name-upsert',
        'service': b'srv',
        'channel': 123,
        'state': None,
    }


async def test_trx_force_failure(service_client, ydb, userver_ydb_trx):
    userver_ydb_trx.enable_failure('trx')
    response = await service_client.post(
        'ydb/upsert-row',
        json={
            'id': 'id-upsert',
            'name': 'name-upsert',
            'service': 'srv',
            'channel': 123,
        },
    )
    assert response.status_code == 500

    cursor = ydb.execute('SELECT * FROM events WHERE id = "id-upsert"')
    assert len(cursor) == 1
    assert len(cursor[0].rows) == 0
