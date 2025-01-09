import pytest

# Executing simple queries pipeline (POST, GET, UPDATE, DELETE)
async def test_basic_crud(service_client):
    # Checking that deleting a row with a certain key, even if there is no such key, the request will be processed correctly
    response = await service_client.delete('/basic/sqlite?key=hello')
    assert response.status == 200

    # Checking the creation of new record
    response = await service_client.post('/basic/sqlite?key=hello&value=world')
    assert response.status == 201
    assert response.text == 'world'

    # Checking for getting a previously created record by key
    response = await service_client.get('/basic/sqlite?key=hello')
    assert response.status == 200
    assert response.text == 'world'

    # Checking for updating a previously created record by key
    response = await service_client.put('/basic/sqlite?key=hello&value=there')
    assert response.status == 200
    assert response.text == 'there'

    # Checking that the database record has been updated
    response = await service_client.get('/basic/sqlite?key=hello')
    assert response.status == 200
    assert response.text == 'there'

    # Checking the deletion by key of a previously created record
    response = await service_client.delete('/basic/sqlite?key=hello')
    assert response.status == 200

    # And after it an attempt to getting data using this key gives an error
    response = await service_client.get('/basic/sqlite?key=hello')
    assert response.status == 404

# Try to insert a new record with an existing key and get a PRIMARY KEY Constraint error
async def test_primary_key_constraint(service_client):
    # Succesful create a new record
    response = await service_client.post('/basic/sqlite?key=hello&value=there')
    assert response.status == 201
    assert response.text == 'there'

    # Creating a record with the same key fails with an error
    response = await service_client.post('/basic/sqlite?key=hello&value=again')
    assert response.status == 409
    assert response.text == 'there'

# Unsuccessful retrieval of a record with an unknown key
async def test_get_unknown_key(service_client):
    # Request with unknown key
    response = await service_client.get('/basic/sqlite?key=unknown')
    assert response.status == 404

# Unsuccessful record update with unknown key
async def test_update_by_unknown_key(service_client):
    # Request with unknown key
    response = await service_client.put('/basic/sqlite?key=unknown&value=foo')
    assert response.status == 404

# A test for working with data batch, inserting and getting several records
# These tests also check work with ResultSet
async def test_batch_select_insert(service_client):
    # Insert 10 pairs and as result get values
    response = await service_client.post(
        '/basic/sqlite/batch/',
        json={'data': [{'key': str(i), 'value': str(i)} for i in range(10)]},
    )
    assert response.status_code == 200
    assert response.json()['values'] == [
        {'key': str(i), 'value': str(i)} for i in range(10)
    ]

    # Get all values
    response = await service_client.get('/basic/sqlite/batch/')
    assert response.status_code == 200
    assert response.json()['values'] == [
        {'key': str(i), 'value': str(i)} for i in range(10)
    ]

# TODO: Improve transaction tests

# A test for checking succesful execute standard transactions with deferred mode
async def test_trx_deffered_ok(service_client):
    response = await service_client.post('/basic/sqlite?key=foo&value=bar')
    assert response.status == 201
    assert response.content == b'bar'

    response = await service_client.get('/basic/sqlite?key=foo')
    assert response.status == 200
    assert response.content == b'bar'

# A test for checking fail execute standard transactions with deferred mode
async def test_trx_fail(service_client):
    response = await service_client.delete('/basic/sqlite?key=foo')
    assert response.status == 200

    # userver_sqlite_trx.enable_failure('sample_transaction_insert_key_value')

    response = await service_client.post('/basic/sqlite?key=foo&value=bar')
    assert response.status == 500

    response = await service_client.get('/basic/sqlite?key=foo')
    assert response.status == 404

# A test for checking standard transactions with immediate mode
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
