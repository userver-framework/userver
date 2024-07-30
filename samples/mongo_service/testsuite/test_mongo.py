# /// [Functional test]
async def test_mongo(service_client):
    data = {
        ('hello', 'ru', 'Привет'),
        ('hello', 'en', 'hello'),
        ('welcome', 'ru', 'Добро пожаловать'),
        ('welcome', 'en', 'Welcome'),
    }
    for key, lang, value in data:
        response = await service_client.patch(
            '/v1/translations',
            params={'key': key, 'lang': lang, 'value': value},
        )
        assert response.status == 201

    response = await service_client.get('/v1/translations')
    assert response.status_code == 200
    assert response.json()['content'] == {
        'hello': {'en': 'hello', 'ru': 'Привет'},
        'welcome': {'ru': 'Добро пожаловать', 'en': 'Welcome'},
    }
    # /// [Functional test]
