async def test_no_cors_headers(service_client):
    response = await service_client.get('/ping')
    assert response.status == 200
    assert 'Access-Control-Allow-Origin' not in response.headers


async def test_preflight(service_client):
    response = await service_client.options(
        '/ping',
        headers={
            'Origin': 'mysite.com',
            'Access-Control-Request-Method': 'GET',
        },
    )
    assert response.status == 204
    assert response.headers['Access-Control-Allow-Origin'] == 'mysite.com'
    assert response.headers['Access-Control-Allow-Methods'] == 'GET'
    assert response.headers['Access-Control-Allow-Credentials'] == 'true'
    assert response.headers['Vary'] == 'Origin'


async def test_origin_bad(service_client):
    response = await service_client.get(
        '/ping',
        headers={
            'Origin': 'example.com',
        },
    )
    assert response.status == 401
    assert response.text == 'Bad Origin header'


async def test_origin_ok(service_client):
    response = await service_client.get(
        '/ping',
        headers={
            'Origin': 'mysite.com',
        },
    )
    assert response.status == 200
    assert response.headers['Access-Control-Allow-Origin'] == 'mysite.com'
    assert response.headers['Vary'] == 'Origin'


async def test_forbidden_method_without_origin(service_client):
    response = await service_client.post(
        '/ping',
    )
    assert response.status == 200


async def test_forbidden_method_with_origin(service_client):
    response = await service_client.post(
        '/ping',
        headers={
            'Origin': 'mysite.com',
        },
    )
    assert response.status == 200
