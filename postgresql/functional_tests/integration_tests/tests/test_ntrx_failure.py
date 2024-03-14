# /// [fault injection]
async def test_ntrx_fail(service_client, pgsql, userver_pg_ntrx):
    response = await service_client.delete('/v1/key-value?key=foo')
    assert response.status == 200

    response = await service_client.post('/v1/key-value?key=foo&value=bar')
    assert response.status == 201

    with userver_pg_ntrx.mock_failure('sample_select_value'):
        response = await service_client.get('/v1/key-value?key=foo')
        assert response.status == 500

    response = await service_client.get('/v1/key-value?key=foo')
    assert response.status == 200
    assert response.content == b'bar'

    # /// [fault injection]
