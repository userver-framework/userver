# /// [Functional test]

# Executing simple queries pipeline (POST, GET, DELETE)
async def test_sqlite(service_client):
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

    # Checking the deletion by key of a previously created record
    response = await service_client.delete('/basic/sqlite/key-value?key=hello')
    assert response.status == 200

    # And after it an attempt to getting data using this key gives an error
    response = await service_client.get('/basic/sqlite/key-value?key=hello')
    assert response.status == 404
    # /// [Functional test]
