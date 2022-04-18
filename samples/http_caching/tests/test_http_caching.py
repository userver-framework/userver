from testsuite import utils


async def test_http_caching(service_client, translations, mocked_time):
    response = await service_client.post(
        '/tests/control',
        json={
            'mock_now': utils.timestring(mocked_time.now()),
            'invalidate_caches': {
                'update_type': 'full',
                'names': ['cache-http-translations'],
            },
        },
    )
    assert response.status_code == 200

    response = await service_client.post(
        '/samples/greet', params={'username': 'дорогой разработчик'},
    )
    assert response.status == 200
    assert response.text == 'Привет, дорогой разработчик! Добро пожаловать'

    translations['hello']['ru'] = 'Приветище'

    mocked_time.sleep(10)
    response = await service_client.post(
        '/tests/control',
        json={
            'mock_now': utils.timestring(mocked_time.now()),
            'invalidate_caches': {
                'update_type': 'incremental',
                'names': ['cache-http-translations'],
            },
        },
    )
    assert response.status_code == 200

    response = await service_client.post(
        '/samples/greet', params={'username': 'дорогой разработчик'},
    )
    assert response.status == 200
    assert response.text == 'Приветище, дорогой разработчик! Добро пожаловать'
