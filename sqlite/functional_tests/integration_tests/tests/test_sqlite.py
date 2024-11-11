async def test_basic_crud(service_client):
    response = await service_client.delete('/basic/sqlite?key=hello')
    assert response.status == 200

    response = await service_client.post('/basic/sqlite?key=hello&value=world')
    assert response.status == 201
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'world'

    response = await service_client.post('/basic/sqlite?key=foo&value=bar')
    assert response.status == 201
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'bar'

    response = await service_client.get('/basic/sqlite?key=hello')
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'world'

    response = await service_client.delete('/basic/sqlite?key=hello')
    assert response.status == 200

    response = await service_client.post('/basic/sqlite?key=hello&value=there')
    assert response.status == 201
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'there'

    response = await service_client.get('/basic/sqlite?key=hello')
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'there'

    response = await service_client.post('/basic/sqlite?key=hello&value=again')
    assert response.status == 409
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'there'

    response = await service_client.get('/basic/sqlite?key=missing')
    assert response.status == 404

async def test_batch_select_insert(service_client):
    response = await service_client.post(
        '/basic/sqlite/batch/',
        json={'data': [{'key': str(i), 'value': str(i)} for i in range(10)]},
    )
    assert response.status_code == 200

    response = await service_client.get('/basic/sqlite/batch/')
    assert response.status_code == 200
    assert 'application/json' in response.headers['Content-Type']
    assert response.json()['values'] == [
        {'key': str(i), 'value': str(i)} for i in range(10)
    ]

async def test_trx_deffered_ok(service_client):
    response = await service_client.post('/basic/sqlite?key=foo&value=bar')
    assert response.status == 201
    assert response.content == b'bar'

    response = await service_client.get('/basic/sqlite?key=foo')
    assert response.status == 200
    assert response.content == b'bar'

async def test_trx_immediate_ok(service_client):
    response = await service_client.post('/basic/sqlite?key=foo&value=bar')
    assert response.status == 201
    assert response.content == b'bar'

    response = await service_client.put('/basic/sqlite?key=foo&value=bar')
    assert response.status == 201
    assert response.content == b'bar'

    response = await service_client.get('/basic/sqlite?key=foo')
    assert response.status == 200
    assert response.content == b'bar'

async def test_trx_exclusive_ok(service_client):
    # test backup, periodic task (summary or deleting old rows)
    pass

async def test_trx_fail(service_client):
    response = await service_client.delete('/basic/sqlite?key=foo')
    assert response.status == 200

    # userver_sqlite_trx.enable_failure('sample_transaction_insert_key_value')

    response = await service_client.post('/basic/sqlite?key=foo&value=bar')
    assert response.status == 500

    response = await service_client.get('/basic/sqlite?key=foo')
    assert response.status == 404
