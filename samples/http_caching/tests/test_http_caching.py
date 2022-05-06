# /// [Functional test]
async def test_http_caching(service_client, translations, mocked_time):
    response = await service_client.post(
        '/samples/greet', params={'username': 'дорогой разработчик'},
    )
    assert response.status == 200
    assert response.text == 'Привет, дорогой разработчик! Добро пожаловать'

    translations['hello']['ru'] = 'Приветище'

    mocked_time.sleep(10)
    await service_client.invalidate_caches()

    response = await service_client.post(
        '/samples/greet', params={'username': 'дорогой разработчик'},
    )
    assert response.status == 200
    assert response.text == 'Приветище, дорогой разработчик! Добро пожаловать'
    # /// [Functional test]
