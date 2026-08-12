# Executing simple queries pipeline (POST, GET, UPDATE, DELETE)
async def test_basic_crud(service_client):
    # Checking that deleting a row with a certain key, even if there is no such key, the request will be processed correctly
    response = await service_client.delete('/basic/sqlite/key-value?key=hello')
    assert response.status == 200

    # Checking the creation of new record
    response = await service_client.post('/basic/sqlite/key-value?key=hello&value=world')
    assert response.status == 201
    assert response.text == 'world'

    # Checking for getting a previously created record by key
    response = await service_client.get('/basic/sqlite/key-value?key=hello')
    assert response.status == 200
    assert response.text == 'world'

    # Try to insert a new record with an existing key and get a PRIMARY KEY Constraint error
    response = await service_client.post('/basic/sqlite/key-value?key=hello&value=new')
    assert response.status == 409

    # Checking for updating a previously created record by key
    response = await service_client.put('/basic/sqlite/key-value?key=hello&value=there')
    assert response.status == 201
    assert response.text == 'there'

    # Checking that the database record has been updated
    response = await service_client.get('/basic/sqlite/key-value?key=hello')
    assert response.status == 200
    assert response.text == 'there'

    # Checking the deletion by key of a previously created record
    response = await service_client.delete('/basic/sqlite/key-value?key=hello')
    assert response.status == 200

    # And after it an attempt to getting data using this key gives an error
    response = await service_client.get('/basic/sqlite/key-value?key=hello')
    assert response.status == 404


# Unsuccessful retrieval of a record with an unknown key
async def test_get_unknown_key(service_client):
    # Request with `unknown` key
    response = await service_client.get('/basic/sqlite/key-value?key=unknown')
    assert response.status == 404

    # Make that key, to make sure that it is not stored for the next test
    response = await service_client.post('/basic/sqlite/key-value?key=unknown&value=unknown_value')
    assert response.status == 201
    assert response.text == 'unknown_value'


async def test_get_unknown_key_again(service_client):
    # Request with `unknown` key, that should be cleaned up in previous step
    response = await service_client.get('/basic/sqlite/key-value?key=unknown')
    assert response.status == 404


# Unsuccessful record update with unknown key
async def test_update_by_unknown_key(service_client):
    response = await service_client.put('/basic/sqlite/key-value?key=unknown&value=foo')
    assert response.status == 404


# A test for checking successful execute standard transactions with deferred mode
async def test_trx_deferred_ok(service_client):
    response = await service_client.delete('/basic/sqlite/key-value?key=foo')
    assert response.status == 200

    response = await service_client.post('/basic/sqlite/key-value?key=foo&value=bar')
    assert response.status == 201
    assert response.content == b'bar'

    response = await service_client.get('/basic/sqlite/key-value?key=foo')
    assert response.status == 200
    assert response.content == b'bar'

    response = await service_client.delete('/basic/sqlite/key-value?key=foo')
    assert response.status == 200


# A test for checking standard transactions with immediate mode
async def test_trx_immediate_ok(service_client):
    response = await service_client.delete('/basic/sqlite/key-value?key=foo')
    assert response.status == 200

    response = await service_client.post('/basic/sqlite/key-value?key=foo&value=bar')
    assert response.status == 201
    assert response.content == b'bar'

    response = await service_client.put('/basic/sqlite/key-value?key=foo&value=bar')
    assert response.status == 201
    assert response.content == b'bar'

    response = await service_client.get('/basic/sqlite/key-value?key=foo')
    assert response.status == 200
    assert response.content == b'bar'

    response = await service_client.delete('/basic/sqlite/key-value?key=foo')
    assert response.status == 200


# A test for working with data batch, inserting and getting several records
# These tests also check work with ResultSet
async def test_batch_select_insert(service_client):
    values = [{'key': str(i), 'value': str(i)} for i in range(10)]

    # Insert 10 pairs and as result get values
    response = await service_client.post(
        '/basic/sqlite/batch/',
        json={'data': values},
    )
    assert response.status_code == 200
    assert response.json()['values'] == values

    # Get all values
    response = await service_client.get('/basic/sqlite/batch/')
    assert response.status_code == 200
    assert response.json()['values'] == values


# TODO: how to check auto-rollback and failure trx logic?
