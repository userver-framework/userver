async def test_trx_ok(service_client, pgsql):
    response = await service_client.post('/v1/key-value?key=foo&value=bar')
    assert response.status == 201
    assert response.content == b'bar'

    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status == 200
    assert response.content == b'bar'


# /// [fault injection]
async def test_trx_fail(service_client, pgsql, userver_pg_trx):
    response = await service_client.delete('/v1/key-value?key=foo')
    assert response.status == 200

    userver_pg_trx.enable_failure('sample_transaction_insert_key_value')

    response = await service_client.post('/v1/key-value?key=foo&value=bar')
    assert response.status == 500

    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status == 404
    # /// [fault injection]
