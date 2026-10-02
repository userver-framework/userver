async def test_server_pipeline(service_client):
    response = await service_client.get('/server-only')

    assert response.status_code == 200
    assert response.text == 'handled'
    assert (
        response.headers['X-Middleware-Order']
        == 'server-prepend-first,server-prepend-second,server-append-first,server-append-second,'
    )


async def test_handler_pipeline(service_client):
    response = await service_client.get('/combined')

    assert response.status_code == 200
    assert response.text == 'handled'
    assert response.headers['X-Middleware-Order'] == (
        'handler-prepend-first,handler-prepend-second,server-prepend-first,server-prepend-second,'
        'server-append-first,server-append-second,handler-append-first,handler-append-second,'
    )
