# /// [Functional test]
async def test_mongo(service_client, mongodb):
    data = {
        ('hello', 'ru', 'Привет'),
        ('hello', 'en', 'hello'),
        ('welcome', 'ru', 'Добро пожаловать'),
        ('welcome', 'en', 'Welcome'),
    }

    # mongodb.get_aliases() shows the available databases
    translations_db = mongodb.translations
    for key, lang, value in data:
        response = await service_client.patch(
            '/v1/translations',
            params={'key': key, 'lang': lang, 'value': value},
        )
        assert response.status == 201

        # Checking content of the database via direct access
        assert translations_db.find_one({'key': key, 'lang': lang})['value'] == value

    response = await service_client.get('/v1/translations')
    assert response.status_code == 200
    assert 'application/json' in response.headers['Content-Type']
    assert response.json()['content'] == {
        'hello': {'en': 'hello', 'ru': 'Привет'},
        'welcome': {'ru': 'Добро пожаловать', 'en': 'Welcome'},
    }
    # /// [Functional test]
