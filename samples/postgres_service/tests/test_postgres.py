# /// [Functional test]
async def test_postgres(service_client, pgsql):
    # POSTing 'key=hello&value=world'
    response = await service_client.post('/v1/key-value?key=hello&value=world')
    assert response.status == 201
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'world'

    # Checking content of the database via direct access.
    # `pgsql.keys()` outputs all the databases
    cursor = pgsql['admin'].cursor()  # 'admin.sql' file contains the schema.
    cursor.execute("SELECT value FROM key_value_table WHERE key='hello'")
    result = cursor.fetchall()
    assert len(result) == 1
    assert result[0][0] == 'world'

    # Getting key=hello
    response = await service_client.get('/v1/key-value?key=hello')
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'world'

    # Deleting key=hello
    response = await service_client.delete('/v1/key-value?key=hello')
    assert response.status == 200

    # Make sure database is does not store key=hello
    cursor.execute("SELECT value FROM key_value_table WHERE key='hello'")
    assert len(cursor.fetchall()) == 0
    # /// [Functional test]

    response = await service_client.post('/v1/key-value?key=hello&value=there')
    assert response.status == 201
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'there'

    response = await service_client.get('/v1/key-value?key=hello')
    assert response.status == 200
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'there'

    response = await service_client.post('/v1/key-value?key=hello&value=again')
    assert response.status == 409
    assert 'text/plain' in response.headers['Content-Type']
    assert response.text == 'there'

    response = await service_client.get('/v1/key-value?key=missing')
    assert response.status == 404
