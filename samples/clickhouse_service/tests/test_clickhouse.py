async def test_clickhouse(service_client, clickhouse):
    response = await service_client.request('GET', '/v1/db?limit=10')
    assert response.status_code == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == '0123456789'

    # Direct database access. `clickhouse.keys()` outputs all the databases
    db = clickhouse['clickhouse-database']
    result = db.execute(
        'SELECT toString(c.number) FROM system.numbers c LIMIT toInt32(3)',
    )
    assert result == [
        (response.text[0],),
        (response.text[1],),
        (response.text[2],),
    ]
